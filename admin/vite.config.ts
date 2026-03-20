import { defineConfig, loadEnv } from 'vite'
import vue from '@vitejs/plugin-vue'
import { fileURLToPath, URL } from 'node:url'
import AutoImport from 'unplugin-auto-import/vite'
import Components from 'unplugin-vue-components/vite'
import { ElementPlusResolver } from 'unplugin-vue-components/resolvers'

function normalizeOrigin(raw: string | undefined, fallback: string): string {
  const value = raw?.trim()
  if (!value) return fallback
  return value.replace(/\/+$/, '')
}

function toWsOrigin(origin: string): string {
  return origin
    .replace(/^https:\/\//, 'wss://')
    .replace(/^http:\/\//, 'ws://')
}

export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, process.cwd(), '')
  const apiOrigin = normalizeOrigin(env.VITE_DEV_API_ORIGIN, 'http://127.0.0.1:8080')
  const wsOrigin = normalizeOrigin(env.VITE_DEV_WS_ORIGIN, toWsOrigin(apiOrigin))

  return {
    base: '/admin/',
    plugins: [
      vue(),
      AutoImport({ resolvers: [ElementPlusResolver()] }),
      Components({ resolvers: [ElementPlusResolver()] }),
    ],
    resolve: {
      alias: { '@': fileURLToPath(new URL('./src', import.meta.url)) }
    },
    server: {
      port: 5173,
      strictPort: true,
      proxy: {
        '/api': { target: apiOrigin, changeOrigin: true },
        '/ws': { target: wsOrigin, ws: true, changeOrigin: true }
      }
    },
    css: {
      preprocessorOptions: {
        scss: { api: 'modern-compiler' },
      },
    },
    build: {
      cssCodeSplit: true,
      chunkSizeWarningLimit: 650,
      rollupOptions: {
        output: {
          manualChunks(id) {
            if (!id.includes('node_modules')) return
            if (id.includes('/echarts/')) return 'echarts'
            if (id.includes('/element-plus/')) return 'element-plus'
            if (id.includes('/dayjs/')) return 'dayjs'
            if (
              id.includes('/vue/') ||
              id.includes('/vue-router/') ||
              id.includes('/pinia/') ||
              id.includes('/axios/')
            ) {
              return 'framework'
            }
          },
        },
      },
    },
  }
})
