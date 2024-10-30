#include <sqlite3.h>
#include <iostream>
#include <cstdint>
#include "usings.h"
#include "orderType.h"
#include "side.h"

class DatabaseConnection
{
    private:
        sqlite3 *DB;
        std::string insertPriceVolDataQuery;
        std::string insertOrderQuery;

    public:
        DatabaseConnection(const char *dbPath)
        {
            int exit = 1;
            exit = sqlite3_open(dbPath, &DB);
            if (exit)
            {
                std::cerr << "Could not open DB at " << dbPath << std::endl;
            }
            insertPriceVolDataQuery = "INSERT INTO pricevoldata (timestamp, volume, price) VALUES (?, ?, ?);";
            insertOrderQuery = "INSERT INTO order (timestamp, ordertype, side, price, quantity) VALUES (?, ?, ?, ?, ?);";
        };
        void InsertPriceVolData(std::string timestamp, int volume, Price price)
        {
            sqlite3_stmt *stmt;
            sqlite3_prepare_v3(DB, insertPriceVolDataQuery.c_str(), -1, -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, timestamp.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, volume);
            sqlite3_bind_int(stmt, 3, price);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        };
        void InsertOrderData(std::string timestamp, OrderType type, Side side, Price price, Quantity quantity)
        {
            sqlite3_stmt *stmt;
            sqlite3_prepare_v3(DB, insertPriceVolDataQuery.c_str(), -1, -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, timestamp.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, (int)type);
            sqlite3_bind_int(stmt, 3, (int)side);
            sqlite3_bind_int(stmt, 4, price);
            sqlite3_bind_int(stmt, 5, quantity);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        };
        ~DatabaseConnection()
        {
            sqlite3_close_v2(DB);
        }
};