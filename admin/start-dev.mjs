import('vite').then(v => v.createServer({server:{host:'0.0.0.0'}}).then(s => s.listen().then(() => {
  console.warn('Admin running at http://localhost:5173');
  s.printUrls();
})));
