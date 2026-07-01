"""
Gera gráfico de Comprimento Médio Progressivo a partir de logs do PPM-C.

Uso:
  python3 plot_compression.py log1.csv [log2.csv ...] [-o saida.png] [-t "Título"]

Exemplos:
  # Um único arquivo (seção 3.2 — Dickens):
  python3 plot_compression.py dickens_semreset.csv -o dickens.png

  # Comparação dos três modos (seção 3.2 e 3.3):
  python3 plot_compression.py sem_reset.csv com_poda.csv com_reset.csv -o comparacao.png
"""

import sys
import os
import argparse
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker


# Parser do CSV

def parse_log(filename):
    posicoes, bps_list, eventos = [], [], []
    modo = "nenhum"
    j_val = None
    threshold = None

    with open(filename, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue

            if line.startswith("#"):
                if "modo:" in line:
                    for part in line.split("|"):
                        part = part.strip().lstrip("#").strip()
                        if part.startswith("modo:"):
                            modo = part[5:].strip()
                        elif part.startswith("j:"):
                            try:
                                j_val = int(part[2:].strip())
                            except ValueError:
                                pass
                        elif part.startswith("threshold:"):
                            try:
                                threshold = int(part[10:].strip().rstrip("%"))
                            except ValueError:
                                pass
                continue

            if line.startswith("posicao"):
                continue

            parts = line.split(",")
            if len(parts) < 2:
                continue
            try:
                pos = int(parts[0])
                bps = float(parts[1])
                evento = parts[2].strip() if len(parts) > 2 else ""
                posicoes.append(pos)
                bps_list.append(bps)
                if evento:
                    eventos.append((pos, evento))
            except ValueError:
                continue

    return {
        "modo": modo,
        "j": j_val,
        "threshold": threshold,
        "posicoes": posicoes,
        "bps": bps_list,
        "eventos": eventos,
    }


# Rótulo automático para cada série

def make_label(data, filename):
    base = os.path.splitext(os.path.basename(filename))[0]
    modo = data["modo"]
    if modo == "nenhum":
        return f"{base} (sem reset/poda)"
    if data["j"] and data["threshold"] is not None:
        return f"{base} ({modo}, j={data['j']}, p={data['threshold']}%)"
    return f"{base} ({modo})"


# Cores

LINE_COLORS = ["#1f77b4", "#2ca02c", "#9467bd", "#8c564b", "#17becf"]
EVENT_COLORS = {"poda": "#ff7f0e", "reset": "#d62728"}


# Plot

def plot(log_files, output=None, title=None):
    fig, ax = plt.subplots(figsize=(13, 6))

    event_labels_added = set()

    for idx, filename in enumerate(log_files):
        data = parse_log(filename)

        if not data["posicoes"]:
            print(f"Aviso: '{filename}' não tem dados válidos.",
                  file=sys.stderr)
            continue

        color = LINE_COLORS[idx % len(LINE_COLORS)]
        label = make_label(data, filename)

        xs = [p / 1000 for p in data["posicoes"]]
        ax.plot(xs, data["bps"], color=color,
                linewidth=1.4, label=label, zorder=3)

        # Linhas verticais nos eventos de poda/reset
        for pos, tipo in data["eventos"]:
            ec = EVENT_COLORS.get(tipo, "gray")
            ev_label = tipo if tipo not in event_labels_added else None
            ax.axvline(
                x=pos / 1000,
                color=ec,
                linestyle="--",
                linewidth=0.9,
                alpha=0.75,
                label=ev_label,
                zorder=2,
            )
            if ev_label:
                event_labels_added.add(tipo)

    ax.set_xlabel("Posição no arquivo (×1000 símbolos)", fontsize=12)
    ax.set_ylabel("Comprimento médio (bits/símbolo)", fontsize=12)
    ax.set_title(title or "Comprimento Médio — PPM-C", fontsize=14)
    ax.legend(fontsize=9, loc="upper right")
    ax.grid(True, alpha=0.25, zorder=1)
    ax.xaxis.set_major_formatter(
        mticker.FuncFormatter(lambda x, _: f"{x:.0f}k")
    )

    plt.tight_layout()

    if output:
        plt.savefig(output, dpi=150)
        print(f"Gráfico salvo em: {output}")
    else:
        plt.show()

def main():
    parser = argparse.ArgumentParser(
        description="Plota comprimento médio progressivo de logs PPM-C."
    )
    parser.add_argument(
        "logs", nargs="+", metavar="log.csv",
        help="Arquivos CSV gerados pela flag -log do compressor",
    )
    parser.add_argument(
        "-o", "--output", metavar="imagem",
        help="Salvar como arquivo (PNG, PDF, SVG…). Omitir para exibir na tela.",
    )
    parser.add_argument(
        "-t", "--title", metavar="texto",
        help="Título do gráfico",
    )
    args = parser.parse_args()

    missing = [f for f in args.logs if not os.path.isfile(f)]
    if missing:
        for f in missing:
            print(f"Erro: arquivo não encontrado: {f}", file=sys.stderr)
        sys.exit(1)

    plot(args.logs, output=args.output, title=args.title)


if __name__ == "__main__":
    main()
