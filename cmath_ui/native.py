#!/usr/bin/env python3
"""Native Qt WebEngine host. Computations stay in the C++ loopback service."""
import json
import os
import sys
import urllib.parse
import urllib.request
from pathlib import Path

try:
    from PySide6.QtCore import QTimer, QUrl
    from PySide6.QtGui import QCloseEvent
    from PySide6.QtWidgets import QApplication, QFileDialog, QMainWindow
    from PySide6.QtWebEngineCore import QWebEnginePage, QWebEngineProfile, QWebEngineSettings
    from PySide6.QtWebEngineWidgets import QWebEngineView
except ImportError:
    raise SystemExit("Native mode requires Python 3 and PySide6 with Qt WebEngine. Install PySide6 or use browser_window().")

if len(sys.argv) != 3:
    raise SystemExit("Usage: native.py local_url window_title")
url, title = sys.argv[1:]
parsed = urllib.parse.urlsplit(url)
if parsed.scheme != 'http' or parsed.hostname != '127.0.0.1' or not parsed.port:
    raise SystemExit('Only the local cmath C++ engine is accepted.')
origin = f'http://127.0.0.1:{parsed.port}'
token = parsed.path.strip('/')

class LocalPage(QWebEnginePage):
    def acceptNavigationRequest(self, target, navigation_type, is_main_frame):
        return target.scheme() == 'http' and target.host() == '127.0.0.1' and target.port() == parsed.port

class MathWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle(title)
        self.resize(1280, 860)
        self.view = QWebEngineView(self)
        self.profile = QWebEngineProfile(self)  # off-the-record; no persistent browser profile
        self.page = LocalPage(self.profile, self.view)
        self.view.setPage(self.page)
        self.page.settings().setAttribute(QWebEngineSettings.WebAttribute.LocalContentCanAccessFileUrls, False)
        self.page.settings().setAttribute(QWebEngineSettings.WebAttribute.JavascriptCanOpenWindows, False)
        self.page.settings().setAttribute(QWebEngineSettings.WebAttribute.JavascriptCanAccessClipboard, True)
        self.profile.downloadRequested.connect(self.download)
        self.setCentralWidget(self.view)
        self.view.load(QUrl(url))
        self.view.loadFinished.connect(self.loaded)

    def download(self, request):
        filename, _ = QFileDialog.getSaveFileName(self, 'Save math session', request.downloadFileName(), 'JSON (*.json);;All files (*)')
        if filename:
            request.setDownloadDirectory(str(Path(filename).parent))
            request.setDownloadFileName(Path(filename).name)
            request.accept()
        else:
            request.cancel()

    def loaded(self, ok):
        if not ok:
            print('Native view could not load the C++ workspace.', file=sys.stderr)
        # Explicit test harness options, inactive during ordinary use.
        smoke_ms = int(os.environ.get('CMATH_UI_SMOKE_MS', '0'))
        if smoke_ms:
            QTimer.singleShot(smoke_ms, self.capture_and_close)

    def capture_and_close(self):
        output = os.environ.get('CMATH_UI_SCREENSHOT')
        if output:
            self.grab().save(output)
        self.page.runJavaScript("document.querySelector('#results')?.innerText || document.body.innerText", self.finish_smoke)

    def finish_smoke(self, text):
        print('NATIVE_SMOKE: ' + json.dumps(text), flush=True)
        self.close()

    def closeEvent(self, event: QCloseEvent):
        try:
            req = urllib.request.Request(url.rstrip('/') + '/shutdown', data=b'', headers={'X-Math-Token': token, 'Origin': origin}, method='POST')
            urllib.request.urlopen(req, timeout=1).close()
        except Exception:
            pass
        event.accept()

app = QApplication(sys.argv[:1])
app.setApplicationName('cmath Math Studio')
window = MathWindow()
window.show()
sys.exit(app.exec())
