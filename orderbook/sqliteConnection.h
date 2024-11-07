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
        std::string getSideRatioQuery;

    public:
        DatabaseConnection(const char *dbPath)
        {
            int exit = 1;
            exit = sqlite3_open(dbPath, &DB);
            if (exit)
            {
                std::cerr << "Could not open DB at " << dbPath << std::endl;
            }
            getSideRatioQuery = "SELECT ratio FROM orderpressure WHERE id = 1;";
            insertPriceVolDataQuery = "INSERT INTO pricevoldata (timestamp, volume, price) VALUES (?, ?, ?);";
            insertOrderQuery = "INSERT INTO orderdata (timestamp, ordertype, side, price, quantity) VALUES (?, ?, ?, ?, ?);";
        };
        void InsertPriceVolData(std::string timestamp, int volume, Price price)
        {
            sqlite3_stmt *stmt;
            sqlite3_prepare_v3(DB, insertPriceVolDataQuery.c_str(), -1, -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, timestamp.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, volume);
            sqlite3_bind_int(stmt, 3, price);
            int s = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        };
        void InsertOrderData(std::string timestamp, OrderType type, Side side, Price price, Quantity quantity)
        {
            sqlite3_stmt *stmt;
            sqlite3_prepare_v3(DB, insertOrderQuery.c_str(), -1, -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, timestamp.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, (int)type);
            sqlite3_bind_int(stmt, 3, (int)side);
            sqlite3_bind_int(stmt, 4, price);
            sqlite3_bind_int(stmt, 5, quantity);
            int s = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        };
        int GetOrderPressureRatio()
        {
            sqlite3_stmt *stmt;
            int result = 500;
            sqlite3_prepare_v3(DB, getSideRatioQuery.c_str(), -1, -1, &stmt, nullptr);
            if (sqlite3_step(stmt) == SQLITE_ROW)
            {
                result = sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
            return result;
        };
        ~DatabaseConnection()
        {
            sqlite3_close_v2(DB);
        }
};