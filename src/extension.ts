
import * as vscode from 'vscode';
import * as path from 'node:path';
import { mkdirSync } from 'node:fs';
import { execFileSync } from 'node:child_process';

export function activate(context: vscode.ExtensionContext) {
  const color = vscode.window.createTextEditorDecorationType({
    isWholeLine: true,
    backgroundColor: 'rgba(255, 138, 196, 0.18)'
  });

  const command = vscode.commands.registerCommand('LongFunctionDetect_Lite.check', () => {
    const editor = vscode.window.activeTextEditor;

    if (!editor || editor.document.languageId !== 'cpp') {
      vscode.window.showInformationMessage('Откройте файл C++.');
      return;
    }

    try {
      const folder = context.globalStorageUri.fsPath;
      mkdirSync(folder, { recursive: true });

      const source = path.join(context.extensionPath, 'src', 'analyzer.cpp');
      const program = path.join(
        folder,
        process.platform === 'win32' ? 'analyzer.exe' : 'analyzer'
      );

      execFileSync('g++', ['-std=c++17', source, '-o', program]);

      const result = execFileSync(program, [], {
        input: editor.document.getText(),
        encoding: 'utf8'
      });

      const ranges: vscode.DecorationOptions[] = [];

      for (const row of result.trim().split('\n')) {
        if (!row.trim()) continue;

        const [first, last, count] = row.trim().split(' ').map(Number);

        ranges.push({
          range: new vscode.Range(
            first - 1, 0,
            last - 1, editor.document.lineAt(last - 1).text.length
          ),
          hoverMessage: `Длина функции: ${count} строк.`
        });
      }

      editor.setDecorations(color, ranges);
      vscode.window.showInformationMessage(
        `Функций длиннее 40 строк: ${ranges.length}`
      );

    } catch (error) {
      vscode.window.showErrorMessage(`Ошибка: ${String(error)}`);
    }
  });

  const button = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left);
  button.text = '(ﾉ◕ヮ◕)ﾉ*:･ﾟ✧';
  button.color = '#ff8ac4';
  button.tooltip = 'Найти функции C++ длиннее 40 строк';
  button.command = 'LongFunctionDetect_Lite.check';
  button.show();

  const change = vscode.workspace.onDidChangeTextDocument(event => {
    for (const editor of vscode.window.visibleTextEditors) {
      if (editor.document === event.document) {
        editor.setDecorations(color, []);
      }
    }
  });

  context.subscriptions.push(color, command, button, change);
}

export function deactivate() {}
