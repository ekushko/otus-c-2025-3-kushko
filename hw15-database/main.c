#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

static const char* database_name = "db.sqlite";

static const char* table_name = "students";

static const char* column_name[] = {
    "id", "name", "age", "math_score", "history_score", "chemistry_score"
};

static const char* column_type[] = {
    "INTEGER PRIMARY KEY ASC AUTOINCREMENT NOT NULL UNIQUE DEFAULT (1)", "TEXT", "INTEGER", "INTEGER", "INTEGER", "INTEGER"
};

static const char* name[] = {
    "TomasAndersen", "PeterParker", "HarryPotter", "BruceWayne", "ClarkKent", "SarahConor", "EllenRipley", "NatashaRomanova", "MaryJane", "AnnaDark"
};

static const int age[] = {
    17, 18, 16, 15, 18, 16, 16, 17, 18, 17
};

static const int math_score[] = {
    70, 65, 85, 95, 100, 85, 55, 90, 55, 92
};

static const int history_score[] = {
    100, 78, 82, 93, 100, 81, 66, 73, 54, 93
};

static const int chemistry_score[] = {
    88, 75, 61, 98, 52, 87, 92, 45, 72, 98
};

#define QUERY_MAX_LEN 256

#define QUERY_STAT "SELECT %s(%s) AS \"%s_%s\" FROM %s"

enum {
    SUM,
    AVG,
    MAX,
    MIN,
    DSP
};

static int callback(void* unused, int argc, char** argv, char** col_name) {
    for(int i = 0; i < argc; i++)
        printf("%s = %s\n", col_name[i], argv[i] ? argv[i] : "NULL");
    printf("\n");
    return 0;
}

void init_db(sqlite3* db) {
    char query[QUERY_MAX_LEN];
    snprintf(query, QUERY_MAX_LEN, "CREATE TABLE IF NOT EXISTS %s (%s %s, %s %s, %s %s, %s %s, %s %s, %s %s);",
             table_name,
             column_name[0], column_type[0],
             column_name[1], column_type[1],
             column_name[2], column_type[2],
             column_name[3], column_type[3],
             column_name[4], column_type[4],
             column_name[5], column_type[5]);

    char* err_msg = NULL;
    int rc = sqlite3_exec(db, query, callback, 0, &err_msg);
    if (rc != SQLITE_OK) {
        printf("SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    for (int i = 0; i < 10; ++i) {
        snprintf(query, QUERY_MAX_LEN, "INSERT OR IGNORE INTO %s VALUES(%d, '%s', %d, %d, %d, %d);",
                 table_name, i + 1, name[i], age[i], math_score[i], history_score[i], chemistry_score[i]);
        rc = sqlite3_exec(db, query, callback, 0, &err_msg);
        if (rc != SQLITE_OK) {
            printf("SQL error: %s\n", err_msg);
            sqlite3_free(err_msg);
        }
    }
}

int get_stat(sqlite3* db, int op, const char* column, const char* table) {
    int res = 0;
    char query[QUERY_MAX_LEN], op_str[4];

    op_str[3] = '\0';
    if (op != DSP) {
        if (op == SUM) {
            strcpy(op_str, "sum");
        } else if (op == AVG) {
            strcpy(op_str, "avg");
        } else if (op == MAX) {
            strcpy(op_str, "max");
        } else if (op == MIN) {
            strcpy(op_str, "min");
        }

        snprintf(query, QUERY_MAX_LEN, QUERY_STAT, op_str, column, op_str, column, table);

        sqlite3_stmt* stmt;
        char* err_msg = NULL;
        int rc = sqlite3_prepare_v2(db, query, -1, &stmt, 0);
        if (rc != SQLITE_OK) {
            printf("SQL error: %s\n", err_msg);
            sqlite3_free(err_msg);
            return -1;
        }
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            res = sqlite3_column_int(stmt, 0);
        }

        sqlite3_finalize(stmt);
    } else if (op == DSP) {
        strcpy(op_str, "dsp");

        int sum = 0, cnt = 0, avg = get_stat(db, AVG, column, table);

        snprintf(query, QUERY_MAX_LEN, "SELECT %s FROM %s", column, table);
        sqlite3_stmt* stmt;
        char* err_msg = NULL;
        int rc = sqlite3_prepare_v2(db, query, -1, &stmt, 0);
        if (rc != SQLITE_OK) {
            printf("SQL error: %s\n", err_msg);
            sqlite3_free(err_msg);
            return -1;
        }

        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            int val = sqlite3_column_int(stmt, 0);
            val -= avg;
            sum += val * val;
            ++cnt;
        }
        res = sum / cnt;
    }
    return res;
}

char* alloc_str_copy(const char* src) {
    char* dst = NULL;
    if (src != NULL) {
        size_t len = strlen(src) + 1;
        dst = malloc(len);
        strcpy(dst, src);
        dst[len-1] = '\0';
    }
    return dst;
}

int main(int argc, char* argv[]) {
    char *db_name = NULL, *column = NULL, *table = NULL;
    if (argc >= 4) {
        column = alloc_str_copy(argv[3]);
    } else {
        column = alloc_str_copy(column_name[2]);
    }
    if (argc >= 3) {
        table = alloc_str_copy(argv[2]);
    } else {
        table = alloc_str_copy(table_name);
    }
    if (argc >= 2) {
        db_name = alloc_str_copy(argv[1]);
    } else {
        db_name = alloc_str_copy(database_name);
    }


    sqlite3* db;
    int rc = sqlite3_open(db_name, &db);
    if (rc) {
        printf("Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return EXIT_FAILURE;
    }

    if (argc <= 3) {
        init_db(db);
    }

    int res = 0;
    res = get_stat(db, SUM, column, table);
    printf("SUM: %d\n", res);
    res = get_stat(db, AVG, column, table);
    printf("AVG: %d\n", res);
    res = get_stat(db, MAX, column, table);
    printf("MAX: %d\n", res);
    res = get_stat(db, MIN, column, table);
    printf("MIN: %d\n", res);
    res = get_stat(db, DSP, column, table);
    printf("DSP: %d\n", res);

    sqlite3_close(db);

    free(column);
    free(table);
    free(db_name);
    return 0;
}
