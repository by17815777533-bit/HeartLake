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

  // 让渲染器自动选择，避免个别浏览器或显卡上 CanvasKit 初始化失败导致白屏。
  if (_flutter && _flutter.buildConfig) {
    _flutter.buildConfig.renderer = "auto";
  }

  _flutter.loader.load();
})();
