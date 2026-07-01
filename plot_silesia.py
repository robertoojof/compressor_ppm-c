"""
Gera gráfico de Comprimento Médio Progressivo para o Corpus Silesia,
com fronteiras entre arquivos calculadas automaticamente.

Uso:
  python3 plot_silesia.py [-o saida.png] [-t "Título"]

Espera encontrar:
  silesia/          — arquivos do corpus (para calcular fronteiras)
  silesia_analise/baseline/k4.csv
  silesia_analise/k4_reset/j50000_r50.csv
  silesia_analise/k4_poda/j10000_p50.csv
"""

import os
import argparse
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker


SILESIA_ORDER = [
    'dickens', 'mozilla', 'mr', 'nci', 'ooffice', 'osdb',
    'reymont', 'samba', 'sao', 'webster', 'xml', 'x-ray',
]

SERIES = [
    ('silesia_analise/baseline/k4.csv',
     'Caso base (sem reset/poda)', '#1f77b4'),
    ('silesia_analise/k4_reset/j50000_r50.csv',
     'Reset (j=50000, p=50%)', '#2ca02c'),
    ('silesia_analise/k4_poda/j10000_p50.csv',
     'Poda halving (j=10000, p=50%)', '#9467bd'),
]


def calc_boundaries(silesia_dir='silesia'):
    """Calcula posições acumuladas (em símbolos) das fronteiras entre arquivos."""
    boundaries, labels = [], []
    cumsum = 0
    for name in SILESIA_ORDER[:-1]:
        path = os.path.join(silesia_dir, name)
        cumsum += os.path.getsize(path)
        boundaries.append(cumsum)
        labels.append(name)
    return boundaries, labels


def parse_log(filename):
    posicoes, bps_list = [], []
    with open(filename, encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#') or line.startswith('posicao'):
                continue
            parts = line.split(',')
            try:
                posicoes.append(int(parts[0]))
                bps_list.append(float(parts[1]))
            except ValueError:
                continue
    return posicoes, bps_list


def plot(output=None, title=None, silesia_dir='silesia'):
    boundaries, bound_labels = calc_boundaries(silesia_dir)

    fig, ax = plt.subplots(figsize=(14, 6))

    for path, label, color in SERIES:
        xs, ys = parse_log(path)
        ax.plot([x / 1000 for x in xs], ys,
                color=color, linewidth=1.4, label=label, zorder=3)

    for b, lbl in zip(boundaries, bound_labels):
        ax.axvline(x=b / 1000, color='gray', linestyle=':',
                   linewidth=0.8, alpha=0.6, zorder=2)
        ax.text(b / 1000, 3.85, lbl, rotation=90, fontsize=6,
                color='gray', ha='right', va='top')

    ax.set_xlabel('Posição no arquivo (×1000 símbolos)', fontsize=12)
    ax.set_ylabel('Comprimento médio progressivo (bits/símbolo)', fontsize=12)
    ax.set_title(title or 'Comprimento Médio Progressivo — Corpus Silesia (K=4)',
                 fontsize=14)
    ax.legend(fontsize=10, loc='upper right')
    ax.grid(True, alpha=0.25, zorder=1)
    ax.set_ylim(1.9, 4.1)
    ax.xaxis.set_major_formatter(
        mticker.FuncFormatter(lambda x, _: f'{x:.0f}k')
    )

    plt.tight_layout()

    if output:
        plt.savefig(output, dpi=150)
        print(f'Gráfico salvo em: {output}')
    else:
        plt.show()


def main():
    parser = argparse.ArgumentParser(
        description='Plota comprimento médio progressivo do Corpus Silesia.'
    )
    parser.add_argument('-o', '--output', metavar='imagem',
                        help='Arquivo de saída (PNG, PDF, SVG…)')
    parser.add_argument('-t', '--title', metavar='texto',
                        help='Título do gráfico')
    parser.add_argument('--silesia-dir', default='silesia',
                        metavar='dir',
                        help='Diretório com os arquivos do Corpus Silesia')
    args = parser.parse_args()
    plot(output=args.output, title=args.title, silesia_dir=args.silesia_dir)


if __name__ == '__main__':
    main()
