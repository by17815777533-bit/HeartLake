# 数据迁移说明

`backend/migrations/` 当前负责数据库结构和基础数据变更。

## 当前执行方式

```bash
psql "$DATABASE_URL" -f backend/migrations/<file>.sql
```

## 当前要求

- 迁移只写结构和基础数据
- 已上线表结构变更前先核对代码引用
- 新字段需要同步更新模型和接口文档
- 删除列前先完成代码清理
