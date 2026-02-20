#!/usr/bin/env python3
"""
pcfg2foma.py

Convert a (P)CFG into a finite-state approximation of its tree language.

This script reads a context-free grammar (optionally weighted) and emits a
*foma* script that compiles an automaton `Gr` approximating the grammar’s
tree language. You can then parse sentences (with weights ignored) using 
foma’s `Parse()` function:

    regex Parse(sentence);

which produces an automaton containing all valid parses.

You can also just do the composition yieldinv .o. Gr to create a transducer
that parses input string into trees.

Grammar format
--------------
One rule per line:

    LHS -> RHS1 ... RHSn [optional_weight]

Example:

    S -> NP VP -0.0003

This also supports multiple RHS alternatives on one line:

    LHS -> RHS1 | RHS2 | ... | RHSn

Each alternative is treated as a separate rule.

Command-line options
--------------------
-i  Number of center-embeddings to approximate (default: 1)
-g  Grammar file (required)
-s  Start symbol (default: S)

Example
-------
Example grammar (example.txt):

    S  -> NP VP
    NP -> D N | NP RC | N
    RC -> NP V
    VP -> V NP | V
    D  -> a | the
    N  -> rat | cat | dog | cheese
    V  -> chased | killed | ate

Approximate the CFG with at most 3 center-embeddings and write a foma script:

    ./pcfg2foma.py -s S -i 3 -g example.txt > example.foma

Load and run in foma:

    foma -l example.foma
    defined Gr: 26.7 kB. 1471 states, 1639 arcs, Cyclic

    foma[1]: set print-space ON
    variable print-space = ON

    foma[1]: regex Parse(the rat the cat the dog chased killed ate the cheese);
    3.8 kB. 177 states, 176 arcs, 1 path.

    foma[2]: words
    [ the ] [ D -> the ] { < [ rat ] ( N -> rat ) > } [ NP -> D N ] { < [ the ] [ D -> the ] { < [ cat ] ( N -> cat ) > } [ NP -> D N ] { < [ the ] [ D -> the ] { < [ dog ] ( N -> dog ) > } [ NP -> D N ] ( RC -> NP V ) [ V -> chased ] [ chased ] > } [ NP -> NP RC ] ( RC -> NP V ) [ V -> killed ] [ killed ] > } [ NP -> NP RC ] ( S -> NP VP ) [ VP -> V NP ] { < [ ate ] ( V -> ate ) > } [ NP -> D N ] { < [ the ] ( D -> the ) > } [ N -> cheese ] [ cheese ]

Reference
---------
Hulden, Mans and Miikka Silfverberg. “Finite-state subset approximation of
phrase structure.” ISAIM 2014, Fort Lauderdale, FL, USA, January 6–8, 2014.

"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from typing import Dict, List, Optional, Sequence, Tuple


_NUM_RE = re.compile(r"^-?[0-9]+(\.[0-9]+)?$")


def _is_weight(tok: str) -> bool:
    return bool(_NUM_RE.match(tok))


@dataclass(frozen=True)
class Rule:
    lhs: str
    rhs: Tuple[str, ...]  # weight removed


def _split_alternatives(rhs_tokens: List[str]) -> List[List[str]]:
    """
    Split tokens on '|' into alternatives.

    Example:
        ["NP", "VP", "|", "VP"]
    ->  [["NP", "VP"], ["VP"]]
    """
    alts: List[List[str]] = [[]]
    for t in rhs_tokens:
        if t == "|":
            alts.append([])
        else:
            alts[-1].append(t)
    # drop empty alts (e.g. malformed "A -> | B")
    return [a for a in alts if a]


def _parse_grammar_lines(lines: Sequence[str]) -> List[Rule]:
    rules: List[Rule] = []
    for raw in lines:
        line = raw.strip()
        if not line or line.startswith("#"):
            continue

        toks = line.split()
        if len(toks) < 3 or toks[1] != "->":
            raise ValueError(f"Bad rule line (expected 'LHS -> RHS...'): {raw.rstrip()}")

        lhs = toks[0]
        rhs_tokens = toks[2:]

        # Support: LHS -> RHS1 | RHS2 | ... | RHSn
        for alt in _split_alternatives(rhs_tokens):
            # Strip optional weight from THIS alternative
            if alt and _is_weight(alt[-1]):
                alt = alt[:-1]
            if not alt:
                raise ValueError(f"Empty RHS alternative in: {raw.rstrip()}")
            rules.append(Rule(lhs=lhs, rhs=tuple(alt)))
    return rules


def _build_foma_script(
    rules: Sequence[Rule],
    grammarfile: str,
    startsymbol: str = "S",
    embeddings: int = 1,
) -> str:
    # Pass 1: figure out nonterminals/terminals
    # seen[sym] = 1 => nonterminal, 2 => terminal (default if only appears on RHS)
    seen: Dict[str, int] = {}

    for r in rules:
        seen[r.lhs] = 1
        for sym in r.rhs:
            if seen.get(sym) not in (1, 2):
                seen[sym] = 2

    # Pass 2: create grammar pieces
    lrules_parts: List[str] = []
    rrules_parts: List[str] = []
    crules_parts: List[str] = []

    for r in rules:
        lhs = r.lhs
        rhs = list(r.rhs)
        rhssize = len(rhs)

        righthandside = "\"->\" " + " ".join(f"\"{s}\"" for s in rhs) + " "

        # rrules begins with a bracketed "header" for each rule
        rrules = f"%[ \"{lhs}\" {righthandside} %] "

        lrules = ""
        crules = ""

        for i, currsym in enumerate(rhs):
            # Helper: nonterminals carry "RHS " decoration
            wstring = "RHS " if seen.get(currsym) != 2 else ""

            # Center
            if i == 0:
                if seen.get(currsym) != 2:
                    crules += f"NOTPB %[ \"{currsym}\" {wstring}%] \"(\" {lhs} {righthandside}\")\" "
                else:
                    crules += f"%[ \"{currsym}\" %] \"(\" {lhs} {righthandside}\")\" "
            else:
                if i > 1:
                    crules += "%^ "
                if seen.get(currsym) != 2:
                    crules += f"%[ \"{currsym}\" {wstring}%] NOTPB "
                else:
                    crules += f"%[ \"{currsym}\" %] "

            # Right
            if i >= 0 and (i + 1) < rhssize:
                rrules += f"%{{ \"{currsym}\" {wstring}%}} "
            else:
                rrules += f"%[ \"{currsym}\" {wstring}%] "

            # Left
            cs = rhs[i]
            wstring_left = "RHS " if seen.get(cs) != 2 else ""
            if i > 0 and i < rhssize:
                lrules += f"%{{ \"{rhs[i]}\" {wstring_left}%}} "
            else:
                lrules += f"%[ \"{rhs[i]}\" {wstring_left}%] "

        lrules += f"%[ \"{lhs}\" {righthandside} %]"
        rrules += ""
        crules += ""

        lrules_parts.append(lrules)
        rrules_parts.append(rrules)
        crules_parts.append(crules)

    # Build NT/T
    terminals: List[str] = []
    nonterminals: List[str] = []
    for sym, typ in seen.items():
        if typ == 2:
            terminals.append(f" \"{sym}\" ")
        else:
            nonterminals.append(f" \"{sym}\" ")

    # Match formatting: join with |
    NT = " |".join(nonterminals).rstrip()
    T = " |".join(terminals).rstrip()

    LRules = " |\n".join(lrules_parts)
    RRules = " |\n".join(rrules_parts)
    Center = " |\n".join(crules_parts)

    out: List[str] = []
    out.append(f"define Nonterminal {NT} ;\n")
    out.append(f"define Terminal {T} ;\n")
    out.append("define RHS \"->\" [Nonterminal|Terminal]+;\n")
    out.append("define PB [%^|\"(\"|\")\"];\n")
    out.append(f"define LRules {LRules} ;\n")
    out.append(f"define RRules {RRules} ;\n")
    out.append(f"define Center {Center} ;\n")
    out.append("define Center [Center .o. [NOTPB:[[\\PB-NOTPB]*]|\\NOTPB]*].l;\n")
    out.append("define Mid [ %[ [Nonterminal RHS | Terminal]  %] | %{ [Nonterminal RHS | Terminal] %} ]*;\n")
    out.append("define Filter [ %[ Terminal %] Mid* \"(\" Nonterminal RHS \")\" [Mid* %[ Terminal %] %^]* (Mid* %[ Terminal %]) ];\n")
    out.append("define LR0 Filter <> %| %|;\n")
    out.append(
        "define LR [[ \\%|* %| %[ \\[%||%]]* %] [%{ \\[%||%}]* %}]* %[ \\[%||%]]* %] %| \\%|* ] & LR0] - \n"
        "[ [ \\%|* %| LRules %| \\%|* \"(\" \\%|* ] | [ \\%|* \")\" \\%|* %| RRules %| \\%|* ] ];\n"
    )
    out.append("define LR2 ~`[LR,%|,0] & Filter & Center;\n")
    out.append("define Alphabet Nonterminal | Terminal | %{ | %} | %[ | %] | %^ | \"->\" ;\n")
    out.append(
        "define IT [\\%{ | %{ %< | %{ 0:[\"<#\" Alphabet*] 0:%( Nonterminal RHS 0:%) 0:[Alphabet* \">#\"] %} | %{ Terminal %} ]* "
        ".o. ~[?* \"<#\" [ [?-\"<#\"-\">#\"]* & ~LR2 ] \">#\" ?*] .o. \"<#\" -> %< , \">#\" -> %>;\n"
    )

    # Gr definition with embeddings
    out.append(f"define Gr [[LR2 & $[\"(\" {startsymbol}]] ")
    for _ in range(int(embeddings)):
        out.append(".o. IT ")
    out.append(".o. ~$[%{ Nonterminal]].l;\n")

    out.append('define yieldinv [["{" "<" | ">" "}"] -> 0 .o. [[ "[":0 [Terminal|EPSILON:0] "]":0 | "{":0 [Terminal|EPSILON:0] "}":0 | "[":0 [Nonterminal:0]+ "->":0 [[Nonterminal|Terminal]:0]+ "]":0 | "(":0 [Nonterminal:0]+ "->":0 [[Nonterminal|Terminal]:0]+ ")":0 | "^":0 ]*]].i;\n')
    out.append('define yield yieldinv.i;\n')
    out.append("define Strings [Gr .o. yield .o. EPSILON -> 0].l;\n")
    out.append("define Parse(String) [String .o. yieldinv].l & Gr;\n")
    out.append("regex Gr;\n")

    return "".join(out)


def main(argv: Optional[Sequence[str]] = None) -> int:
    ap = argparse.ArgumentParser(
        prog="pcfg2foma",
        description="Convert a (P)CFG to a foma script for Hulden & Silfverberg CFG subset approximation.",
    )
    ap.add_argument("-g", "--grammar-file", required=True, help="Grammar file (one rule per line).")
    ap.add_argument("-s", "--start-symbol", default="S", help="Start symbol (default: S).")
    ap.add_argument("-i", "--embeddings", type=int, default=1, help="Number of center-embeddings approximated (default: 1).")

    args = ap.parse_args(argv)

    with open(args.grammar_file, "r", encoding="utf-8") as f:
        lines = f.readlines()

    rules = _parse_grammar_lines(lines)
    script = _build_foma_script(
        rules=rules,
        grammarfile=args.grammar_file,
        startsymbol=args.start_symbol,
        embeddings=args.embeddings,
    )
    sys.stdout.write(script)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

