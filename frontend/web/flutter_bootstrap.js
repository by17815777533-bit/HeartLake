{{flutter_js}}
{{flutter_build_config}}

(async function () {
  const host = window.location.hostname || '';
  const isPrivateIp = /^(127\.0\.0\.1|0\.0\.0\.0|10\.\d{1,3}\.\d{1,3}\.\d{1,3}|192\.168\.\d{1,3}\.\d{1,3}|172\.(1[6-9]|2\d|3[0-1])\.\d{1,3}\.\d{1,3})$/.test(host);
  const isLocal = host === 'localhost' || isPrivateIp;

  // 本地开发时主动清理 Service Worker，避免旧缓存脚本导致白屏。
  if (isLocal && 'serviceWorker' in navigator) {
    try {
      const regs = await navigator.serviceWorker.getRegistrations();
      await Promise.all(regs.map((r) => r.unregister()));
    } catch (_) {}
  }

  // 本地优先走本地 CanvasKit，避免外网/CDN 波动导致首屏卡死。
  const loadConfig = {};
  const primaryBuild = _flutter?.buildConfig?.builds?.[0];
  if (isLocal && primaryBuild?.compileTarget === 'dart2js') {
    loadConfig.canvasKitBaseUrl = 'canvaskit/';
  }

  _flutter.loader.load({config: loadConfig});
})();
