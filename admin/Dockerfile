# HeartLake 管理后台 — 多阶段构建
# 阶段1: 编译环境
FROM node:20-alpine AS builder

WORKDIR /app

COPY package.json package-lock.json ./
RUN npm ci --no-audit --no-fund

COPY . .
RUN npm run build

# 阶段2: 最小运行时镜像
FROM nginx:1.27-alpine AS runtime

RUN addgroup -S heartlake && adduser -S heartlake -G heartlake

COPY --from=builder /app/dist /usr/share/nginx/html
COPY nginx.conf /etc/nginx/conf.d/default.conf

RUN chown -R heartlake:heartlake /usr/share/nginx/html \
    && chown -R heartlake:heartlake /var/cache/nginx \
    && chown -R heartlake:heartlake /var/log/nginx \
    && touch /var/run/nginx.pid \
    && chown heartlake:heartlake /var/run/nginx.pid

USER heartlake

EXPOSE 80

HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
    CMD wget -qO- http://localhost:80/ || exit 1

CMD ["nginx", "-g", "daemon off;"]
