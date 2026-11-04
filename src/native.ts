import { existsSync } from 'node:fs';
import { mkdir } from 'node:fs/promises';
import * as path from 'node:path';
import { execFile } from 'node:child_process';
import { promisify } from 'node:util';

const runCompiler = promisify(execFile);

export function analyzerName(): string {
  return process.platform === 'win32' ? 'analyzer-v2.exe' : 'analyzer-v2';
}

export async function buildAnalyzer(
  extensionFolder: string,
  outputFolder: string
): Promise<string> {
  await mkdir(outputFolder, { recursive: true });

  const source = path.join(extensionFolder, 'src', 'analyzer.cpp');
  const binary = path.join(outputFolder, analyzerName());
  const compiler = process.env.CXX || 'g++';

  const args = [
    '-std=c++17',
    '-Wall',
    '-Wextra',
    '-pedantic',
    source,
    '-o',
    binary
  ];

  if (process.platform === 'win32') {
    args.push('-static-libgcc', '-static-libstdc++');
  }

  try {
    await runCompiler(compiler, args, { timeout: 60000 });
  } catch (error: unknown) {
    const message = error instanceof Error
      ? error.message
      : String(error);

    throw new Error(
      'Не удалось собрать C++. Проверьте g++ и PATH. ' + message
    );
  }

  return binary;
}

export async function getAnalyzer(
  extensionFolder: string,
  storageFolder: string
): Promise<string> {
  const local = path.join(extensionFolder, 'bin', analyzerName());

  if (existsSync(local)) {return local;}

  const saved = path.join(storageFolder, analyzerName());

  if (existsSync(saved)) {return saved;}

  return buildAnalyzer(extensionFolder, storageFolder);
}

export function runAnalyzer(
  binary: string,
  text: string
): Promise<string> {
  return new Promise<string>((resolve, reject) => {
    const child = execFile(
      binary,
      [],
      { timeout: 10000, encoding: 'utf8' },
      (error, output) => {
        if (error) {
          reject(error);
        } else {
          resolve(output);
        }
      }
    );

    const input = child.stdin;

    if (!input) {
      child.kill();
      reject(new Error('Не удалось передать текст в C++.'));
      return;
    }

    input.on('error', reject);
    input.end(text);
  });
}