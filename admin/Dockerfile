# HeartLake Admin Panel - Nginx serving pre-built Vue3 assets
FROM nginx:alpine

COPY dist/ /usr/share/nginx/html/
COPY nginx.conf /etc/nginx/conf.d/default.conf

EXPOSE 80
