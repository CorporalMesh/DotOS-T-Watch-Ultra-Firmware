#!/usr/bin/env python3
"""Create an offline WebSerial installer from four separate ESP32-S3 images.

Arguments: <build-dir> <boot_app0.bin> <source-commit> <bundled-web-tools.js> <output.html>
The caller bundles the vendored ESP Web Tools module; no runtime CDN is used.
"""
import base64
import hashlib
import html
import json
from pathlib import Path
import sys

build, boot_app0, commit, module, output = sys.argv[1:6]
parts = []
rows = []
for name, path, offset in (
    ('bootloader.bin', Path(build)/'bootloader.bin', 0),
    ('partitions.bin', Path(build)/'partitions.bin', 0x8000),
    ('boot_app0.bin', Path(boot_app0), 0xE000),
    ('firmware.bin', Path(build)/'firmware.bin', 0x10000),
):
    data = path.read_bytes()
    if not data:
        raise ValueError(f'Empty image: {name}')
    digest = hashlib.sha256(data).hexdigest()
    parts.append(dict(name=name, offset=offset, b64=base64.b64encode(data).decode(), sha256=digest))
    rows.append(f'<tr><td>0x{offset:X}</td><td>{name}</td><td>{len(data):,}</td></tr>')
bundle = Path(module).read_text().replace('</script', '<\\/script')
page = '''<!doctype html><html lang="en"><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>DotOS MeshCore installer</title>
<style>
:root{color-scheme:dark}body{margin:0;background:#101115;color:#eee;font:17px/1.6 system-ui}
main{max-width:700px;margin:auto;padding:36px 22px}h1{line-height:1.2}a{color:#9acfff}
section{background:#1b1d24;padding:22px;border-radius:14px;margin:24px 0}
button[slot=activate]{font:600 18px system-ui;padding:14px 24px;border:0;border-radius:8px;background:#e54a45;color:white;cursor:pointer}
li{margin:12px 0}table{width:100%;font-size:14px}td{text-align:left;padding:6px;border-bottom:1px solid #444}
.note{color:#bec3ce;font-size:15px}code{overflow-wrap:anywhere}
</style><main>
<h1>DotOS MeshCore installer</h1>
<p>MeshCore 1.18 experimental alpha · LILYGO T-Watch Ultra (SX1262)</p>
<p class="note">Source: COMMIT. Firmware compiles and passes host checks. Alpha 1 validation on a new T-Watch Ultra was reported on November 3, 2026.</p>
<section><esp-web-install-button id="installer">
<button slot="activate">Install on watch</button>
<span slot="unsupported">Open this file in desktop Chrome or Edge. This browser does not support WebSerial.</span>
<span slot="not-allowed">Download and open this file directly in desktop Chrome or Edge.</span>
</esp-web-install-button><p id="status">Preparing firmware…</p></section>
<ol>
<li>Open this file in desktop <strong>Chrome or Edge</strong>. Close other serial monitors or flashers.</li>
<li><strong>Hold BOOT while connecting</strong> the watch with a USB-C data cable, then release BOOT.</li>
<li>Click <strong>Install on watch</strong>, select the watch's USB serial port, and choose <strong>Install DotOS MeshCore</strong>.</li>
<li>Leave <strong>Erase device</strong> unchecked to preserve settings. Wait for installation to finish.</li>
<li>Press RESET or reconnect USB. On the watch enable <strong>LoRa</strong>, tap the <strong>MeshCore screen title</strong>, enable <strong>Phone Sync</strong>, and enter its PIN in the MeshCore phone app.</li>
</ol>
<p class="note">If the serial connection fails, hold BOOT and tap RESET, then retry. The first start may take longer than usual. If your firmware uses a different partition layout, use the four included images together.</p>
<section><h2>Included firmware</h2><table><tr><th>Offset</th><th>Image</th><th>Bytes</th></tr>ROWS</table>
<p class="note">All firmware and installer code are embedded in this file. It works offline and writes four separate images, preserving flash mode/frequency/size headers. No merged image.</p></section>
<p><a href="https://github.com/CorporalMesh/DotOS-T-Watch-Ultra-Firmware/blob/codex/meshcore-1.18/docs/MESHCORE.md">Instructions and companion limitations</a></p>
<p>Contributor: <a href="https://columbiagorgemesh.org">Columbia Gorge Mesh</a>.</p>
</main><script>
const PARTS = PARTS_JSON;
const files = PARTS.map(p => {
 const raw=atob(p.b64), bytes=new Uint8Array(raw.length);
 for(let i=0;i<raw.length;i++)bytes[i]=raw.charCodeAt(i);
 return {path:URL.createObjectURL(new Blob([bytes],{type:'application/octet-stream'})),offset:p.offset};
});
const manifest={name:'DotOS MeshCore',version:'VERSION',new_install_prompt_erase:true,builds:[{chipFamily:'ESP32-S3',parts:files}]};
document.getElementById('installer').setAttribute('manifest',URL.createObjectURL(new Blob([JSON.stringify(manifest)],{type:'application/json'})));
document.getElementById('status').textContent='Ready: four firmware images loaded.';
</script><script type="module">BUNDLE</script></html>'''
page = page.replace('COMMIT', html.escape(commit[:12])).replace('VERSION', html.escape(commit[:7]))
page = page.replace('ROWS', ''.join(rows)).replace('PARTS_JSON', json.dumps(parts)).replace('BUNDLE', bundle)
Path(output).write_text(page)
print(f'Created {output}: {len(page.encode()):,} bytes')
