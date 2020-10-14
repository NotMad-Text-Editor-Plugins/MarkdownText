# MarkDownText

- A new markdown plugin based on [Miniblink](https://github.com/weolar/miniblink49), [Libcef](https://github.com/chromiumembedded/cef) and [Webview2](https://github.com/MicrosoftEdge/WebView2Samples) 
- Being part of the Textrument Project, this repo need to be compiled against [Textrument]()
- Compatible with Notepad++ v7. 

Top Features
- Alternative browser kernels and Markdown Engines.
- Realtime updating.
- Sync scrolls.

Entry : MarkDownTextDlg::display( where brower widgets are created )、MarkDownTextDlg::refreshWebview( where virtual pages are loaded )

Embeded Markdown Engines

- [md.html](https://github.com/MakeNowJust/md.html)
- [MDViewer](https://github.com/volca/markdown-preview)

How to customize a Markdown Engines
1. create a new folder under notepad/plugins/MarkdownText, with the name of NewMdViewer.
2. create and edit main.js under the `NewMdViewer` folder. this is the entry file that will be loaded by the plugin ( when NewMdViewer is selected as Markdown Engine ).
3. After main.js has finished, a `window.APMD(markdown text)` function must be exported. the plugin will use this function to convert and append the output html.

How to support Scrolls-syncing
- The Markdown Engine should scatter some top-level `<span ln='123'></span>` across the html output, see [MDViewer]. 

WIP…