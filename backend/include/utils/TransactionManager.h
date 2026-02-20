/**
 * @file TransactionManager.h
 * @brief 事务管理器 - 确保数据库操作的ACID特性
 * @author 白洋
 * @date 2026-02-08
 */

#pragma once

#include <drogon/drogon.h>
#include <functional>
#include <memory>

namespace heartlake {
namespace utils {

/**
 * @brief 事务结果
 */
template<typename T>
struct TransactionResult {
    bool success;
    T data;
    std::string error;

    static TransactionResult<T> ok(const T& value) {
        return {true, value, ""};
    }

    static TransactionResult<T> fail(const std::string& errorMsg) {
        return {false, T{}, errorMsg};
    }

    operator bool() const { return success; }
};

/**
 * @brief 事务管理器
 *
 * 提供数据库事务的统一管理，确保ACID特性：
 * - Atomicity (原子性): 事务中的所有操作要么全部成功，要么全部失败
 * - Consistency (一致性): 事务执行前后数据保持一致
 * - Isolation (隔离性): 并发事务之间相互隔离
 * - Durability (持久性): 事务提交后数据永久保存
 */
class TransactionManager {
public:
    /**
     * @brief 执行事务（同步版本）
     *
     * @tparam T 返回值类型
     * @param dbClient 数据库客户端
     * @param transaction 事务函数，接收事务对象作为参数
     * @return TransactionResult<T> 事务执行结果
     *
     * @example
     * auto result = TransactionManager::execute<std::string>(
     *     dbClient,
     *     [](const drogon::orm::DbClientPtr& trans) -> std::string {
     *         // 执行多个数据库操作
     *         trans->execSqlSync("INSERT INTO ...");
     *         trans->execSqlSync("UPDATE ...");
     *         return "success";
     *     }
     * );
     */
    template<typename T>
    static TransactionResult<T> execute(
        const drogon::orm::DbClientPtr& dbClient,
        std::function<T(const drogon::orm::DbClientPtr&)> transaction
    ) {
        try {
            // 开始事务
            auto transPtr = dbClient->newTransaction();

            try {
                // 执行事务函数
                T result = transaction(transPtr);

                // 提交事务
                transPtr->commit();

                LOG_DEBUG << "Transaction committed successfully";
                return TransactionResult<T>::ok(result);

            } catch (const std::exception& e) {
                // 回滚事务
                transPtr->rollback();
                LOG_ERROR << "Transaction rolled back: " << e.what();
                return TransactionResult<T>::fail(std::string("Transaction failed: ") + e.what());
            }

        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to create transaction: " << e.what();
            return TransactionResult<T>::fail(std::string("Failed to create transaction: ") + e.what());
        }
    }

    /**
     * @brief 执行事务（异步版本）
     *
     * @tparam T 返回值类型
     * @param dbClient 数据库客户端
     * @param transaction 事务函数
     * @param callback 完成回调
     *
     * @example
     * TransactionManager::teAsync<std::string>(
     *     dbClient,
     *     [](const drogon::orm::DbClientPtr& trans, auto resolve, auto reject) {
     *         trans->execSqlAsync("INSERT INTO ...",
     *             [resolve](const drogon::orm::Result&) { resolve("success"); },
     *             [reject](const drogon::orm::DrogonDbException& e) { reject(e.base().what()); }
     *         );
     *     },
     *     [](const TransactionResult<std::string>& result) {
     *         if (result) {
     *             LOG_INFO << "Transaction succeeded: " << result.data;
     *         } else {
     *             LOG_ERROR << "Transaction failed: " << result.error;
     *         }
     *     }
     * );
     */
    template<typename T>
    static void executeAsync(
        const drogon::orm::DbClientPtr& dbClient,
        std::function<void(
            const drogon::orm::DbClientPtr&,
            std::function<void(const T&)>,
            std::function<void(const std::string&)>
        )> transaction,
        std::function<void(const TransactionResult<T>&)> callback
    ) {
        try {
            auto transPtr = dbClient->newTransaction();

            // 定义 resolve 和 reject 函数
            auto resolve = [transPtr, callback](const T& result) {
                try {
                    transPtr->commit();
                    LOG_DEBUG << "Async transaction committed successfully";
                    callback(TransactionResult<T>::ok(result));
                } catch (const std::exception& e) {
                    LOG_ERROR << "Failed to commit transaction: " << e.what();
                    callback(TransactionResult<T>::fail(std::string("Commit failed: ") + e.what()));
                }
            };

            auto reject = [transPtr, callback](const std::string& error) {
                try {
                    transPtr->rollback();
                    LOG_ERROR << "Async transaction rolled back: " << error;
                    callback(TransactionResult<T>::fail(error));
                } catch (const std::exception& e) {
                    LOG_ERROR << "Failed to rollback transaction: " << e.what();
                    callback(TransactionResult<T>::fail(std::string("Rollback failed: ") + e.what()));
                }
            };

            // 执行事务函数
            transaction(transPtr, resolve, reject);

        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to create async transaction: " << e.what();
            callback(TransactionResult<T>::fail(std::string("Failed to create transaction: ") + e.what()));
        }
    }

    /**
     * @brief 执行简单事务（无返回值）
     *
     * @param dbClient 数据库客户端
     * @param transaction 事务函数
     * @return bool 是否成功
     */
    static bool executeSimple(
        const drogon::orm::DbClientPtr& dbClient,
        std::function<void(const drogon::orm::DbClientPtr&)> transaction
    ) {
        auto result = execute<bool>(dbClient, [transaction](const drogon::orm::DbClientPtr& trans) {
            transaction(trans);
            return true;
        });
        return result.success;
    }

    /**
     * @brief 批量执行SQL语句（事务中）
     *
     * @param dbClient 数据库客户端
     * @param sqlStatements SQL语句列表
     * @return bool 是否全部成功
     */
    static bool executeBatch(
        const drogon::orm::DbClientPtr& dbClient,
        const std::vector<std::string>& sqlStatements
    ) {
        return executeSimple(dbClient, [&sqlStatements](const drogon::orm::DbClientPtr& trans) {
            for (const auto& sql : sqlStatements) {
                trans->execSqlSync(sql);
            }
        });
    }

    /**
     * @brief 设置事务隔离级别
     *
     * @param trans 事务对象
     * @param level 隔离级别
     * - READ UNCOMM未提交
     * - READ COMMITTED: 读已提交（PostgreSQL默认）
     * - REPEAEAD: 可重复读
     * - SERIALIZABLE: 串行化
     */
    static void setIsolationLevel(
        const drogon::orm::DbClientPtr& trans,
        const std::string& level = "READ COMMITTED"
    ) {
        try {
            trans->execSqlSync("SET TRANSACTION ISOLATION LEVEL " + level);
            LOG_DEBUG << "Transaction isolation level set to: " << level;
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to set isolation level: " << e.what();
        }
    }

    /**
     * @brief 创建保存点
     *
     * @param trans 事务对象
     * @param savepointName 保存点名称
     */
    static void createSavepoint(
        const drogon::orm::DbClientPtr& trans,
        const std::string& savepointName
    ) {
        try {
            trans->execSqlSync("SAVEPOINT " + savepointName);
            LOG_DEBUG << "Savepoint created: " << savepointName;
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to create savepoint: " << e.what();
        }
    }

    /**
     * @brief 回滚到保存点
     *
     * @param trans 事务对象
     * @param savepointName 保存点名称
     */
    static void rollbackToSavepoint(
        const drogon::orm::DbClientPtr& trans,
        const std::string& savepointName
    ) {
        try {
            trans->execSqlSync("ROLLBACK TO SAVEPOINT " + savepointName);
            LOG_DEBUG << "Rolled back to savepoint: " << savepointName;
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to rollback to savepoint: " << e.what();
        }
    }

    /**
     * @brief 释放保存点
     *
     * @param trans 事务对象
     * @param savepointName 保存点名称
     */
    static void releaseSavepoint(
        const drogon::orm::DbClientPtr& trans,
        const std::string& savepointName
    ) {
        try {
            trans->execSqlSync("RELEASE SAVEPOINT " + savepointName);
            LOG_DEBUG << "Savepoint released: " << savepointName;
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to release savepoint: " << e.what();
        }
    }
};

/**
 * @brief 事务作用域守卫（RAII）
 *
 * 自动管理事务的开始、提交和回滚
 *
 * @example
 * {
 *     TransactionGuard guard(dbClient);
 *     auto trans = guard.getTransaction();
 *
 *     trans->execSqlSync("INSERT INTO ...");
 *     trans->execSqlSync("UPDATE ...");
 *
 *     guard.commit(); // 手动提交
 * } // 如果未提交，析构时自动回滚
 */
class TransactionGuard {
public:
    explicit TransactionGuard(const drogon::orm::DbClientPtr& dbClient)
        : trans_(dbClient->newTransaction()), committed_(false) {
        LOG_DEBUG << "Transaction started";
    }

    ~TransactionGuard() {
        if (!committed_) {
            try {
                trans_->rollback();
                LOG_WARN << "Transaction auto-rolled back (not committed)";
            } catch (const std::exception& e) {
                LOG_ERROR << "Failed to rollback in destructor: " << e.what();
            }
        }
    }

    // 禁止拷贝
    TransactionGuard(const TransactionGuard&) = delete;
    TransactionGuard& operator=(const TransactionGuard&) = delete;

    /**
     * @brief 获取事务对象
     */
    drogon::orm::DbClientPtr getTransaction() const {
        return trans_;
    }

    /**
     * @brief 提交事务
     */
    void commit() {
        if (!committed_) {
            trans_->commit();
            committed_ = true;
            LOG_DEBUG << "Transaction committed";
        }
    }

    /**
     * @brief 回滚事务
     */
    void rollback() {
        if (!committed_) {
            trans_->rollback();
            committed_ = true;
            LOG_DEBUG << "Transaction rolled back";
        }
    }

private:
    drogon::orm::DbClientPtr trans_;
    bool committed_;
};

} // namespace utils
} // namespace heartlake
