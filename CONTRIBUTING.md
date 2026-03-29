# 贡献指南

这份文档只描述 HeartLake 当前仓库的开发、验证和提交流程。

## 工作范围

- `backend/`：后端控制器、应用层、领域层、基础设施层
- `frontend/`：Flutter 客户端
- `admin/`：Vue 3 管理端
- `docs/`：现行手册
- `scripts/`：构建、部署、验证脚本

## 当前工作方式

1. 先确认当前线上事实和文档口径一致。
2. 先修契约和状态源，再修页面和提示。
3. 过时分叉直接删除，不保留假兜底。
4. 代码改动和文档改动同一次提交完成。

## 提交前检查

```bash
./scripts/verify-2c2g.sh
./scripts/docker-test.sh
cd admin && npm run lint && npm run build
cd backend && cmake --build build-2c2g -j2
cd frontend && flutter analyze
git diff --check
```

## 当前代码约束

- 不吞异常。
- 不用默认值伪装成功。
- 集合接口必须显式返回 `items / total / page / page_size`。
- 实时事件必须显式返回 `type / event / timestamp`。
- 页面、Provider、数据源不能各自维护一套真相。
- 下线路径不再写进功能文档。

## 文档约束

- 只写现状，不写历史。
- 云端事实统一使用 `http://121.41.195.165` 和 `/root/HeartLake`。
- 内部链接使用相对路径，不写本地绝对路径。
- 变更能力、接口、部署流程时必须同步更新 `README.md` 和相关手册。
