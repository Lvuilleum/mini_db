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

static Pager* pager_open(const char* filename);
static void* get_page(Pager* pager, uint32_t page_num);
static int pager_flush(Pager* pager, uint32_t page_num, uint32_t size);
static void pager_close(Pager* pager);
static void* row_slot(Table* table, uint32_t row_num);
static int find_row_index(Table* table, int id, Row* out_row, uint32_t* out_index);

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
    free(table);
}

int write_row(Table* table, const Row* row)
{
    void* destination;

    if (table->num_rows >= MAX_ROWS) {
        return 0;
    }

    destination = row_slot(table, table->num_rows);
    if (destination == NULL) {
        return 0;
    }

    memcpy(destination, row, ROW_SIZE);
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

    if (row_num >= table->num_rows) {
        return 0;
    }

    destination = row_slot(table, row_num);
    if (destination == NULL) {
        return 0;
    }

    memcpy(destination, row, ROW_SIZE);
    return 1;
}

int delete_row(Table* table, int id)
{
    Row row;
    uint32_t index;

    if (!find_active_row_by_id(table, id, &row, &index)) {
        return 0;
    }

    row.is_deleted = 1;
    return write_row_at(table, index, &row);
}

int id_exists(Table* table, int id)
{
    return find_active_row_by_id(table, id, NULL, NULL);
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

int find_active_row_by_id(Table* table, int id, Row* out_row, uint32_t* out_index)
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

static int find_row_index(Table* table, int id, Row* out_row, uint32_t* out_index)
{
    Row row;
    uint32_t i;

    for (i = 0; i < table->num_rows; i++) {
        if (!read_row(table, i, &row)) {
            return 0;
        }
        if (row.id == id && row.is_deleted == 0) {
            if (out_row != NULL) {
                *out_row = row;
            }
            if (out_index != NULL) {
                *out_index = i;
            }
            return 1;
        }
    }

    return 0;
}
