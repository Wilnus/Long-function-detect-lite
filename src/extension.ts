import * as vscode from 'vscode';

import { getAnalyzer, runAnalyzer } from './native.ts';

export function activate(context: vscode.ExtensionContext): void {
  // Внешний вид выделения.
  const decoration = vscode.window.createTextEditorDecorationType({
    isWholeLine: true,
    backgroundColor: 'rgba(255, 138, 196, 0.18)',
    overviewRulerColor: '#ff8ac4',
    overviewRulerLane: vscode.OverviewRulerLane.Right
  });

  let busy = false;

  const command = vscode.commands.registerCommand(
    'LongFunctionDetect.check',
    async () => {
      if (busy) {return;}

      const editor = vscode.window.activeTextEditor;

      if (!editor || editor.document.languageId !== 'cpp') {
        vscode.window.showInformationMessage('Откройте файл C++.');
        return;
      }

      busy = true;

      const document = editor.document;
      const version = document.version;
      const text = document.getText();

      editor.setDecorations(decoration, []);

      try {
        const binary = await getAnalyzer(
          context.extensionPath,
          context.globalStorageUri.fsPath
        );

        const output = await runAnalyzer(binary, text);

        if (document.isClosed || document.version !== version) {
          vscode.window.showInformationMessage(
            'Файл изменён. Повторите проверку.'
          );
          return;
        }

        const ranges: vscode.DecorationOptions[] = [];

        for (const row of output.trim().split(/\r?\n/)) {
          if (!row) {continue;}

          const [first, last, count] = row.split(' ').map(Number);

          ranges.push({
            range: new vscode.Range(
              first - 1,
              0,
              last - 1,
              document.lineAt(last - 1).text.length
            ),
            hoverMessage: 'Длина функции: ' + count + ' строк.'
          });
        }

        editor.setDecorations(decoration, ranges);

        vscode.window.showInformationMessage(
          'Функций длиннее 40 строк: ' + ranges.length
        );
      } catch (error: unknown) {
        const message = error instanceof Error
          ? error.message
          : String(error);

        vscode.window.showErrorMessage('Pink Functions: ' + message);
      } finally {
        busy = false;
      }
    }
  );

  // Кнопка в нижней строке состояния.
  const button = vscode.window.createStatusBarItem(
    vscode.StatusBarAlignment.Left,
    100
  );

  button.text = '(ﾉ◕ヮ◕)ﾉ*:･ﾟ✧';
  button.color = '#ff8ac4';
  button.tooltip = 'Найти функции C++ длиннее 40 строк';
  button.command = 'LongFunctionDetect.check';
  button.show();

  // Сброс выделения при изменении документа.
  const changes = vscode.workspace.onDidChangeTextDocument(event => {
    for (const editor of vscode.window.visibleTextEditors) {
      if (editor.document.uri.toString() === event.document.uri.toString()) {
        editor.setDecorations(decoration, []);
      }
    }
  });

  context.subscriptions.push(decoration, command, button, changes);
}

export function deactivate(): void {}