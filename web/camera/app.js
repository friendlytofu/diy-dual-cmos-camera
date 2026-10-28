(() => {
  'use strict';
  const $ = id => document.getElementById(id);
  const demo = location.protocol === 'file:' || ['localhost', '127.0.0.1'].includes(location.hostname);
  const state = { cards: { outward: [], selfie: [] }, selected: new Set() };
  const key = (camera, name) => camera + ':' + name;
  const fileNumber = name => +(name.match(/_(\d+)\.JPG$/i)?.[1] || 0);
  const prettyBytes = n => n < 1024 ? n + ' B' : n < 1048576 ? (n / 1024).toFixed(1) + ' KB' : (n / 1048576).toFixed(1) + ' MB';
  const fileUrl = (camera, name) => `/api/file?camera=${camera}&name=${encodeURIComponent(name)}`;
  async function api(path, options) { const response = await fetch(path, { cache: 'no-store', ...options }); if (!response.ok) throw Error((await response.text()) || 'Card request failed'); return response; }
  function notify(message) { const el = $('toast'); el.textContent = message; el.classList.add('show'); clearTimeout(notify.timer); notify.timer = setTimeout(() => el.classList.remove('show'), 2200); }
  function showLarge(image) { $('viewerImage').src = image.src; $('viewerImage').alt = image.alt; $('viewer').hidden = false; }
  function hideLarge() { $('viewer').hidden = true; $('viewerImage').removeAttribute('src'); }
  function allFiles() { return ['outward', 'selfie'].flatMap(camera => state.cards[camera].map(file => ({ camera, ...file }))); }
  function updateSelection() {
    const count = state.selected.size, files = allFiles(), available = new Set(files.map(file => key(file.camera, file.name)));
    const selected = [...state.selected].filter(id => available.has(id)).length;
    $('selectionCount').textContent = count + ' selected'; $('downloadSelected').disabled = !count;
    $('selectAll').checked = !!files.length && selected === files.length; $('selectAll').indeterminate = selected > 0 && selected < files.length;
  }
  function render(camera) {
    const gallery = $(camera + 'Gallery'), files = state.cards[camera]; gallery.replaceChildren();
    $(camera + 'State').textContent = `${files.length} photograph${files.length === 1 ? '' : 's'}`;
    if (!files.length) { const p = document.createElement('p'); p.className = 'empty'; p.textContent = 'No saved photographs found on this card.'; gallery.append(p); updateSelection(); return; }
    files.forEach(file => {
      const id = key(camera, file.name), card = document.createElement('article'); card.className = 'photo' + (state.selected.has(id) ? ' selected' : '');
      const preview = document.createElement('button'); preview.className = 'preview preview-button'; preview.type = 'button'; preview.textContent = demo ? (camera === 'outward' ? 'front' : 'selfie') : 'Load preview';
      if (!demo) preview.onclick = () => { const image = document.createElement('img'); image.className = 'preview'; image.alt = file.name; image.onload = () => showLarge(image); image.onclick = () => showLarge(image); image.src = fileUrl(camera, file.name) + '&inline=1'; preview.replaceWith(image); };
      const footer = document.createElement('footer'), check = document.createElement('input'), details = document.createElement('div'), name = document.createElement('strong'), meta = document.createElement('small'), link = document.createElement('a');
      check.type = 'checkbox'; check.checked = state.selected.has(id); check.ariaLabel = 'Select ' + file.name;
      check.onchange = () => { check.checked ? state.selected.add(id) : state.selected.delete(id); render(camera); };
      details.className = 'details'; name.textContent = file.name; meta.textContent = prettyBytes(file.size); details.append(name, meta);
      link.href = fileUrl(camera, file.name); link.download = file.name; link.textContent = 'download'; footer.append(check, details, link); card.append(preview, footer); gallery.append(card);
    }); updateSelection();
  }
  async function loadCard(camera) {
    $(camera + 'State').textContent = 'Reading card…'; $(camera + 'Gallery').innerHTML = `<p class="empty">Reading the ${camera === 'outward' ? 'front' : 'selfie'} microSD card…</p>`;
    if (demo) { state.cards[camera] = Array.from({ length: camera === 'outward' ? 5 : 4 }, (_, i) => ({ name: (camera === 'outward' ? 'OUT_' : 'SELF_') + (18 - i) + '.JPG', size: 228000 + i * 19000 })); render(camera); return; }
    try { const data = await (await api('/api/files?camera=' + camera)).json(); state.cards[camera] = (data.files || []).sort((a, b) => fileNumber(b.name) - fileNumber(a.name)); render(camera); if (data.truncated) notify('Showing the first 400 ' + camera + ' files'); }
    catch (error) { state.cards[camera] = []; $(camera + 'State').textContent = 'Card unavailable'; $(camera + 'Gallery').innerHTML = '<p class="empty">' + error.message + '</p>'; updateSelection(); }
  }
  async function refresh() { state.selected.clear(); await Promise.all([loadCard('outward'), loadCard('selfie')]); }
  async function saveDownload(entry) {
    const response = await api(fileUrl(entry.camera, entry.name));
    const blob = await response.blob(), url = URL.createObjectURL(blob), a = document.createElement('a');
    a.href = url; a.download = entry.name; document.body.append(a); a.click(); a.remove();
    setTimeout(() => URL.revokeObjectURL(url), 2000);
  }
  async function downloadSelected() {
    const entries = [...state.selected].map(id => { const split = id.indexOf(':'); return { camera: id.slice(0, split), name: id.slice(split + 1) }; });
    const button = $('downloadSelected'); button.disabled = true;
    for (let i = 0; i < entries.length; ++i) {
      button.textContent = `Downloading ${i + 1} of ${entries.length}`;
      try { await saveDownload(entries[i]); } catch (error) { notify(`${entries[i].name}: ${error.message}`); button.textContent = 'Download selected'; updateSelection(); return; }
    }
    button.textContent = 'Download selected'; updateSelection();
    if (entries.length) notify(entries.length === 1 ? 'Download complete' : 'Downloads complete');
  }
  function bind() {
    $('refresh').onclick = refresh;
    $('selectAll').onchange = event => { const ids = allFiles().map(file => key(file.camera, file.name)); if (event.target.checked) ids.forEach(id => state.selected.add(id)); else ids.forEach(id => state.selected.delete(id)); render('outward'); render('selfie'); };
    $('downloadSelected').onclick = downloadSelected;
    $('viewerClose').onclick = hideLarge;
    $('viewer').onclick = event => { if (event.target === $('viewer')) hideLarge(); };
    document.addEventListener('keydown', event => { if (event.key === 'Escape') hideLarge(); });
  }
  async function start() {
    bind();
    if (demo) { $('connection').classList.add('online'); $('connection').querySelector('span').textContent = 'interface demo'; }
    else try { await api('/api/status'); $('connection').classList.add('online'); $('connection').querySelector('span').textContent = 'cards connected'; } catch (_) { $('connection').querySelector('span').textContent = 'offline'; }
    await refresh();
  }
  start();
})();
