#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "storage.h"

#define ROW_SIZE ((uint32_t)sizeof(Row))
#define INDEX_EMPTY 0
#define INDEX_OCCUPIED 1
#define INDEX_DELETED 2
#define INDEX_LOAD_FACTOR_NUM 7
#define INDEX_LOAD_FACTOR_DEN 10

static Pager* pager_open(const char* filename);
static void* get_page(Pager* pager, uint32_t page_num);
static int pager_flush(Pager* pager, uint32_t page_num, uint32_t size);
static void pager_close(Pager* pager);
static void* row_slot(Table* table, uint32_t row_num);
static int find_row_index(Table* table, uint32_t id, Row* out_row, uint32_t* out_index);
static uint32_t next_power_of_two(uint32_t value);
static uint32_t hash_uint32(uint32_t key);
static int index_init(Table* table);
static void index_free(Table* table);
static int index_get_row_num(Table* table, uint32_t id, uint32_t* out_row_num);
static int index_put_row_num(Table* table, uint32_t id, uint32_t row_num);
static void index_remove(Table* table, uint32_t id);
static int index_rebuild(Table* table);



Table* db_open(const char* filename)
{
    Pager* pager = pager_open(filename);
    Table* table = malloc(sizeof(Table));

    if (table == NULL) {
        perror("malloc");
        pager_close(pager);
        exit(EXIT_FAILURE);
    }

    table->pager = pager;
    table->num_rows = pager->file_length / ROW_SIZE;

    if (!index_init(table)) {
        pager_close(pager);
        free(table);
        exit(EXIT_FAILURE);
    }

    if (!index_rebuild(table)) {
        index_free(table);
        pager_close(pager);
        free(table);
        exit(EXIT_FAILURE);
    }

    return table;
}



void db_close(Table* table)
{
    uint32_t num_full_pages;
    uint32_t num_additional_rows;
    uint32_t i;

    if (table == NULL) {
        return;
    }

    num_full_pages = table->num_rows / ROWS_PER_PAGE;
    for (i = 0; i < num_full_pages; i++) {
        if (!pager_flush(table->pager, i, PAGE_SIZE)) {
            fprintf(stderr, "Failed to flush page %u\n", i);
            exit(EXIT_FAILURE);
        }
        free(table->pager->pages[i]);
        table->pager->pages[i] = NULL;
    }

    num_additional_rows = table->num_rows % ROWS_PER_PAGE;
    if (num_additional_rows > 0) {
        uint32_t page_num = num_full_pages;
        uint32_t size = num_additional_rows * ROW_SIZE;

        if (!pager_flush(table->pager, page_num, size)) {
            fprintf(stderr, "Failed to flush partial page %u\n", page_num);
            exit(EXIT_FAILURE);
        }
        free(table->pager->pages[page_num]);
        table->pager->pages[page_num] = NULL;
    }

    pager_close(table->pager);
    index_free(table);
    free(table);
}


IndexEntry* get_index_entry_ptr(Table* table, uint32_t slot)
{
    uint32_t entries_per_page = PAGE_SIZE / sizeof(IndexEntry);
    uint32_t page_num = INDEX_START_PAGE + (slot / entries_per_page);
    uint32_t offset_in_page = (slot % entries_per_page) * sizeof(IndexEntry);

    void* page = get_page(table->pager, page_num);
    return (IndexEntry*)((char*)page + offset_in_page);
}

int write_row(Table* table, const Row* row)
{
    void* destination;
    uint32_t row_num;
    uint32_t page_num;
    uint32_t row_offset;
    uint32_t flush_size;

    if (table->num_rows >= MAX_ROWS) {
        return 0;
    }

    destination = row_slot(table, table->num_rows);
    if (destination == NULL) {
        return 0;
    }

    memcpy(destination, row, ROW_SIZE);
    if (!row->is_deleted && !index_put_row_num(table, row->id, table->num_rows)) {
        return 0;
    }
    /* Flush the affected page to disk so data persists immediately */
    row_num = table->num_rows;
    page_num = row_num / ROWS_PER_PAGE;
    row_offset = row_num % ROWS_PER_PAGE;
    flush_size = (row_offset + 1) * ROW_SIZE;

    if (!pager_flush(table->pager, page_num, flush_size)) {
        return 0;
    }

    table->num_rows++;
    return 1;
}

int read_row(Table* table, uint32_t row_num, Row* row)
{
    void* source;

    if (row_num >= table->num_rows) {
        return 0;
    }

    source = row_slot(table, row_num);
    if (source == NULL) {
        return 0;
    }

    memcpy(row, source, ROW_SIZE);
    return 1;
}

int write_row_at(Table* table, uint32_t row_num, const Row* row)
{
    void* destination;
    Row old_row;
    int had_old_row;

    if (row_num >= table->num_rows) {
        return 0;
    }

    had_old_row = read_row(table, row_num, &old_row);

    destination = row_slot(table, row_num);
    if (destination == NULL) {
        return 0;
    }

    memcpy(destination, row, ROW_SIZE);

    if (had_old_row && !old_row.is_deleted) {
        index_remove(table, old_row.id);
    }

    if (!row->is_deleted && !index_put_row_num(table, row->id, row_num)) {
        if (had_old_row) {
            memcpy(destination, &old_row, ROW_SIZE);
            if (!old_row.is_deleted) {
                (void)index_put_row_num(table, old_row.id, row_num);
            }
        }
        return 0;
    }

    /* Flush the full page containing this row */
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    if (!pager_flush(table->pager, page_num, PAGE_SIZE)) {
        return 0;
    }

    return 1;
}

int delete_row(Table* table, uint32_t id)
{
    Row row;
    uint32_t index;

    if (!find_active_row_by_id(table, id, &row, &index)) {
        return 0;
    }

    row.is_deleted = 1;
    return write_row_at(table, index, &row);
}

int id_exists(Table* table, uint32_t id)
{
    uint32_t row_num;
    return index_get_row_num(table, id, &row_num);
}

int count_active_rows(Table* table)
{
    Row row;
    uint32_t i;
    int count = 0;

    for (i = 0; i < table->num_rows; i++) {
        if (!read_row(table, i, &row)) {
            return count;
        }
        if (!row.is_deleted) {
            count++;
        }
    }

    return count;
}

int find_active_row_by_id(Table* table, uint32_t id, Row* out_row, uint32_t* out_index)
{
    return find_row_index(table, id, out_row, out_index);
}

static Pager* pager_open(const char* filename)
{
    int fd;
    off_t file_length;
    Pager* pager;
    uint32_t i;

    fd = open(filename, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    file_length = lseek(fd, 0, SEEK_END);
    if (file_length == (off_t)-1) {
        perror("lseek");
        close(fd);
        exit(EXIT_FAILURE);
    }

    pager = malloc(sizeof(Pager));
    if (pager == NULL) {
        perror("malloc");
        close(fd);
        exit(EXIT_FAILURE);
    }

    pager->file_descriptor = fd;
    pager->file_length = (uint32_t)file_length;
    pager->num_pages = (pager->file_length + PAGE_SIZE - 1) / PAGE_SIZE;

    for (i = 0; i < TABLE_MAX_PAGES; i++) {
        pager->pages[i] = NULL;
    }

    return pager;
}

static void* get_page(Pager* pager, uint32_t page_num)
{
    uint32_t num_pages_on_disk;
    void* page;

    if (page_num >= TABLE_MAX_PAGES) {
        fprintf(stderr, "Tried to fetch page number out of bounds: %u\n", page_num);
        return NULL;
    }

    if (pager->pages[page_num] == NULL) {
        page = malloc(PAGE_SIZE);
        if (page == NULL) {
            perror("malloc");
            return NULL;
        }
        memset(page, 0, PAGE_SIZE);

        num_pages_on_disk = (pager->file_length + PAGE_SIZE - 1) / PAGE_SIZE;
        if (page_num < num_pages_on_disk) {
            ssize_t bytes_read;
            off_t offset = (off_t)page_num * PAGE_SIZE;

            if (lseek(pager->file_descriptor, offset, SEEK_SET) == (off_t)-1) {
                perror("lseek");
                free(page);
                return NULL;
            }

            bytes_read = read(pager->file_descriptor, page, PAGE_SIZE);
            if (bytes_read < 0) {
                perror("read");
                free(page);
                return NULL;
            }
        }

        pager->pages[page_num] = page;

        if (page_num >= pager->num_pages) {
            pager->num_pages = page_num + 1;
        }
    }

    return pager->pages[page_num];
}

static int pager_flush(Pager* pager, uint32_t page_num, uint32_t size)
{
    off_t offset;

    if (page_num >= TABLE_MAX_PAGES || pager->pages[page_num] == NULL) {
        return 0;
    }

    offset = (off_t)page_num * PAGE_SIZE;
    if (lseek(pager->file_descriptor, offset, SEEK_SET) == (off_t)-1) {
        perror("lseek");
        return 0;
    }

    if (write(pager->file_descriptor, pager->pages[page_num], size) != (ssize_t)size) {
        perror("write");
        return 0;
    }

    /* Update file length if we extended the file */
    off_t end_pos = offset + (off_t)size;
    if ((uint32_t)end_pos > pager->file_length) {
        pager->file_length = (uint32_t)end_pos;
    }

    /* Ensure data is committed to disk */
    if (fsync(pager->file_descriptor) == -1) {
        perror("fsync");
        return 0;
    }

    return 1;
}

static void pager_close(Pager* pager)
{
    if (close(pager->file_descriptor) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    }

    free(pager);
}

static void* row_slot(Table* table, uint32_t row_num)
{
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    uint32_t row_offset = row_num % ROWS_PER_PAGE;
    uint32_t byte_offset = row_offset * ROW_SIZE;
    void* page = get_page(table->pager, page_num);

    if (page == NULL) {
        return NULL;
    }

    return (char*)page + byte_offset;
}

static int find_row_index(Table* table, uint32_t id, Row* out_row, uint32_t* out_index)
{
    Row row;
    uint32_t row_num;

    if (!index_get_row_num(table, id, &row_num)) {
        return 0;
    }

    if (!read_row(table, row_num, &row) || row.is_deleted || row.id != id) {
        return 0;
    }

    if (out_row != NULL) {
        *out_row = row;
    }
    if (out_index != NULL) {
        *out_index = row_num;
    }

    return 1;
}

static uint32_t next_power_of_two(uint32_t value)
{
    uint32_t result = 1;

    while (result < value) {
        if (result > (UINT32_MAX / 2)) {
            return 0;
        }
        result <<= 1;
    }

    return result;
}

static uint32_t hash_uint32(uint32_t key)
{
    uint32_t x = key;

    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;

    return x;
}

static int index_init(Table* table)
{
    uint32_t min_capacity;
    uint32_t capacity;

    min_capacity = (MAX_ROWS * INDEX_LOAD_FACTOR_DEN) / INDEX_LOAD_FACTOR_NUM;
    if (min_capacity < 16) {
        min_capacity = 16;
    }

    capacity = next_power_of_two(min_capacity);
    if (capacity == 0) {
        return 0;
    }

    table->id_index = calloc(capacity, sizeof(IndexEntry));
    if (table->id_index == NULL) {
        perror("calloc");
        return 0;
    }

    table->id_index_capacity = capacity;
    return 1;
}

static void index_free(Table* table)
{
    free(table->id_index);
    table->id_index = NULL;
    table->id_index_capacity = 0;
}

static int index_get_row_num(Table* table, uint32_t id, uint32_t* out_row_num)
{
    uint32_t mask;
    uint32_t slot;
    uint32_t probes;

    if (table->id_index == NULL || table->id_index_capacity == 0 || out_row_num == NULL) {
        return 0;
    }

    mask = table->id_index_capacity - 1;
    slot = hash_uint32(id) & mask;

    for (probes = 0; probes < table->id_index_capacity; probes++) {
        IndexEntry* entry = &table->id_index[slot];

        if (entry->state == INDEX_EMPTY) {
            return 0;
        }

        if (entry->state == INDEX_OCCUPIED && entry->key == id) {
            *out_row_num = entry->row_num;
            return 1;
        }

        slot = (slot + 1) & mask;
    }

    return 0;
}

static int index_put_row_num(Table* table, uint32_t id, uint32_t row_num)
{
    uint32_t mask;
    uint32_t slot;
    uint32_t probes;
    uint32_t first_deleted = UINT32_MAX;

    if (table->id_index == NULL || table->id_index_capacity == 0) {
        return 0;
    }

    mask = table->id_index_capacity - 1;
    slot = hash_uint32(id) & mask;

    for (probes = 0; probes < table->id_index_capacity; probes++) {
        IndexEntry* entry = &table->id_index[slot];

        if (entry->state == INDEX_EMPTY) {
            uint32_t target = (first_deleted != UINT32_MAX) ? first_deleted : slot;
            table->id_index[target].key = id;
            table->id_index[target].row_num = row_num;
            table->id_index[target].state = INDEX_OCCUPIED;
            return 1;
        }

        if (entry->state == INDEX_DELETED) {
            if (first_deleted == UINT32_MAX) {
                first_deleted = slot;
            }
        } else if (entry->key == id) {
            entry->row_num = row_num;
            return 1;
        }

        slot = (slot + 1) & mask;
    }

    if (first_deleted != UINT32_MAX) {
        table->id_index[first_deleted].key = id;
        table->id_index[first_deleted].row_num = row_num;
        table->id_index[first_deleted].state = INDEX_OCCUPIED;
        return 1;
    }

    return 0;
}

static void index_remove(Table* table, uint32_t id)
{
    uint32_t mask;
    uint32_t slot;
    uint32_t probes;

    if (table->id_index == NULL || table->id_index_capacity == 0) {
        return;
    }

    mask = table->id_index_capacity - 1;
    slot = hash_uint32(id) & mask;

    for (probes = 0; probes < table->id_index_capacity; probes++) {
        IndexEntry* entry = &table->id_index[slot];

        if (entry->state == INDEX_EMPTY) {
            return;
        }

        if (entry->state == INDEX_OCCUPIED && entry->key == id) {
            entry->state = INDEX_DELETED;
            return;
        }

        slot = (slot + 1) & mask;
    }
}

static int index_rebuild(Table* table)
{
    Row row;
    uint32_t i;
    uint32_t existing_row_num;

    for (i = 0; i < table->num_rows; i++) {
        if (!read_row(table, i, &row)) {
            return 0;
        }
        if (!row.is_deleted && !index_get_row_num(table, row.id, &existing_row_num)) {
            if (!index_put_row_num(table, row.id, i)) {
                return 0;
            }
        }
    }

    return 1;
}
