#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Tommy McLeroy. Part of Fighting Vipers Recomp: https://github.com/tmcleroy/fighting-vipers-recomp
# Fighting Vipers: every rewritten routine (src/fight.cpp, src/round.cpp, src/fightinput.cpp, src/throws.cpp) against the game's own code
# (satrecomp/docs/READABLE.md). A movie of fights is replayed with SATRECOMP_LIFT_CHECK=1, once as it is and
# then with bytes of the game's memory set before each call of a routine, to send it down the ways the
# movie does not reach: other modes, time running out, draws, the deciding round, a match being over, throws.
# Fails if any compared call differs; lists what no run reached.
#
#   games/fighting_vipers/check_rewrites.sh [FRAMES]        (from the repository's root; make check-rewrites)
# Environment: B (the build's folder) and SR (the toolkit), as make gives them.
set -u
GAME=${B:-build/fighting_vipers}/fighting_vipers
DISC=${DISC:-$(ls disc/*.cue | head -1)}
MOVIE=games/fighting_vipers/movies/own_auto_7.txt
FRAMES=${1:-5200}
OUT=$(mktemp -d)
JOBS=${JOBS:-6}
OUTER=FightStep,FightersTask,InputTask,PlayerInput1,PlayerInput2

# name:address=byte,...[;name:...] (the addresses are in src/fv.h). A line starting with -name[,name] runs
# those routines as the game's code, so that the routines they call are met and checked on their own.
POKES='
InputTask
InputTask:060E005B=20
PlayerInput1
PlayerInput1:06062757=02
PlayerInput1:06060157=01
PlayerInput2
PlayerInput2:06062797=01
PlayerInput2:060E0001=03
PlayerInput2:06061457=01
InputStep:06063A1B=01,06063A1D=05
InputStep:06063A1B=01,06063A1D=00
InputStep:06062757=07
InputStep:06062757=07,060639F8=08
InputStep:060627D6=04
InputStep:06062803=01
InputStep:06062803=01,06062801=04,060639F8=01
InputStep:06062803=01,06062801=08,060639F8=02
InputStep:06062803=01,06062800=01,06062801=08,060639F8=01
InputStep:06062803=01,06062800=01,060639F8=04
InputStep:06062803=01,06062800=01,060639F8=04,0606309B=7F
InputStep:06063A00=00,06063A01=00,06063A02=00,06063A03=08
FightersTask
FightersTask:060E005B=20
FightersTask:060E0022=05
FightStep
FightStep:060E0067=20
FightStep:060E0067=20,060E0001=02,060943A4=08
FightStep:060E0067=20,060E0001=01,060943BC=08
FightStep:060E0067=20,060E0001=01,060943A4=08
FightStep:060E0067=20,060E0027=02
FightStep:060E005B=20
RoundClock:060E005C=80
RoundCountdown:060E0067=10
TakeDamage (hit):060E005C=80
TakeDamage (hit):060E0016=03
Begin:060E002C=01
LetGo:060E0002=07
LetGo:060E0002=04
RoundOverWait:060E0002=07
RoundSetUp:060E0002=07
RoundSetUp:060E0067=10
RoundSetUp:060E0067=10,002000A0=01
RoundSetUp:002000A0=01,002000A1=01
RoundSetUp:060E0002=04,06095809=01,06090205=01
RoundSetUp:060E0002=04,06095809=02,06090205=01
RoundSetUp:060E0001=00
RoundSetUp:060E0001=02,060E0027=03
RoundSetUp:060E0001=02,060E0002=02
-RoundOver Fighting:060E0067=01
-RoundOver Fighting:060E0067=01;JudgeTimeUp:06062828=00,06062829=10,06064728=00,06064729=10
-RoundOver Fighting:060E0067=01;JudgeTimeUp:06062828=00,06062829=10,06064728=00,06064729=20
-RoundOver Fighting:060E0067=01;JudgeTimeUp:06062828=00,06062829=20,06064728=00,06064729=10
-RoundClock,RoundSetUp
RoundOver:060E0067=01
RoundOver:060E0067=01,06062829=01
RoundOver:060E0067=01,06064729=01
RoundOver:060E0067=01,06062828=00,06062829=FA,06064728=00,06064729=01
RoundOver:060E0067=11
RoundOver:060E0067=10,060E000F=03,060E0001=01
RoundOver:060E0067=10,060E000F=03,060E0001=02
RoundOver:060E0067=10,060E000F=03,060E0001=03,060E000E=01
RoundOver:060E0067=10,060E000F=03,060E0001=03,060E000E=02
RoundOver:060E0067=10,060E000F=03,060E0001=03,060E000E=00
RoundOver:060E0067=00,060E000F=03
RoundOver:060E0067=00,060E000F=03,060E0008=01,00200E28=02
RoundOver:060E0067=00,060E000F=03,060E0009=01,00200E28=02
RoundOver:060E0067=00,060E000F=03,060E0008=01,00200E28=05
RoundOver:060E0067=00,060E000F=00
RoundOver:060E0067=00,060E000F=01
RoundOver:060E0067=00,060E000F=02,06062828=00,06062829=FA
RoundOver:060E0067=00,060E000F=01,06064728=00,06064729=FA
RoundOver:060E0067=00,060E000F=02,00200E28=01,060E0027=03,060E0001=01,0020004B=40
RoundOver:060E0067=00,060E000F=01,00200E28=01,060E0027=03,060E0001=01
RoundOver:060E0067=00,060E000F=02,00200E28=01,060E0002=03
RoundOver:060E0067=00,060E000F=01,00200E28=01,060E0002=03
RoundOver:060E0067=00,060E000F=02,00200E28=01,060E0002=04,0609580A=01
RoundOver:060E0067=00,060E000F=02,00200E28=01,060E0002=04,0609580A=03
RoundOver:060E0067=00,060E000F=01,00200E28=01,060E0002=04,060957E2=01
RoundOver:060E0067=00,060E000F=01,00200E28=01,060E0002=04,060957E2=03
AnswerThrows:06062810=10,06062812=00,06062813=15
AnswerThrows:06064710=10,06064712=00,06064713=15
AnswerThrows:06062810=10,06062812=00,06062813=15,06064710=10,06064712=00,06064713=15,060E005F=00
AnswerThrows:06062810=10,06062812=00,06062813=15,06064710=10,06064712=00,06064713=15,060E005F=01
AnswerThrows:06062810=27,06062812=00,06062813=15
AnswerThrows:06064710=27,06064712=00,06064713=15
AnswerThrows:06062810=27,06062812=00,06062813=15,06064710=27,06064712=00,06064713=15,060E005F=00
AnswerThrows:06062810=27,06062812=00,06062813=15,06064710=27,06064712=00,06064713=15,060E005F=01
AnswerThrows:06062810=25,06062812=00,06062813=15
AnswerThrows:06064710=25,06064712=00,06064713=15
AnswerThrows:06062810=25,06062812=00,06062813=15,06064710=25,06064712=00,06064713=15,060E005F=00
AnswerThrows:06062810=25,06062812=00,06062813=15,06064710=25,06064712=00,06064713=15,060E005F=01
AnswerThrows:06062810=2B,06062812=00,06062813=15
AnswerThrows:06064710=2B,06064712=00,06064713=15
AnswerThrows:06062810=2B,06062812=00,06062813=15,06064710=2B,06064712=00,06064713=15,060E005F=00
AnswerThrows:06062810=2B,06062812=00,06062813=15,06064710=2B,06064712=00,06064713=15,060E005F=01
AnswerThrows:06062810=10,06062812=00,06062813=15,060627D7=44
AnswerThrows:06062810=25,06062812=00,06062813=15,06064702=10
AnswerThrows:06062810=2B,06062812=00,06062813=15,060646D6=08
AnswerThrows:06064710=2B,06064712=00,06064713=15,060627D6=08
AnswerThrows:06062838=10
AnswerThrows:06064738=10
-AnswerThrows
-AnswerThrows PendingRequest:0606283B=0E
-AnswerThrows PendingRequest:0606283F=01
-AnswerThrows PendingRequest:0606283F=01,06062826=00,06062827=05,06062CD6=00,06062CD7=05
-AnswerThrows PendingRequest:06062810=1C
-AnswerThrows PendingRequest:060627D6=04,06062810=0D
-AnswerThrows PendingRequest:060627D6=04,06062810=23
-AnswerThrows PendingRequest:060627D6=04,06062810=05
'

run() {
    n=$1
    poke=$2
    frames=$3
    # (The OUTER routines call others that are rewritten: with an outer one checked, those run as the game's
    # code on both sides and are not met themselves. So the outer ones are the game's code except in the
    # runs that are for them: the lines that start with their name.)
    skip=$OUTER
    first=${poke%%:*}
    case "$poke" in
    -*)
        skip=${poke%% *}
        skip=$OUTER,${skip#-}
        case "$poke" in *" "*) poke=${poke#* } ;; *) poke= ;; esac
        ;;
    *)
        case ",$OUTER," in
        *",$first,"*)
            skip=$(echo "$OUTER" | tr ',' '\n' | grep -vx "$first" | tr '\n' ',')
            [ "$poke" = "$first" ] && poke=
            ;;
        esac
        ;;
    esac
    SATRECOMP_LIFT_SKIP="$skip" SATRECOMP_LIFT_POKE="$poke" SATRECOMP_LIFT_CHECK=1 SATRECOMP_LIFT_COVERAGE="$OUT/coverage.$n" \
        "$GAME" --disc "$DISC" --headless --frames "$frames" --movie "$MOVIE" 2>&1 | grep -a "^rewrite" | sed "s|^|[$2] |" >"$OUT/log.$n"
}

n=0
run 0 "" 15000 &
echo "$POKES" | {
    while IFS= read -r poke; do
        [ -z "$poke" ] && continue
        n=$((n + 1))
        run $n "$poke" "$FRAMES" &
        [ $((n % JOBS)) -eq 0 ] && wait
    done
    wait
}
wait

cat "$OUT"/log.* | grep -a "differs\|cannot be compared" | head -40
cat "$OUT"/log.* | grep -a " calls, " | grep -av ": 0 differ; 0 could not be compared" | head -20
python3 "${SR:-satrecomp}"/tools/liftcoverage.py "$OUT"/coverage.*
calls=$(cat "$OUT"/log.* | grep -a " calls, " | sed 's/.* calls, \([0-9]*\) compared.*/\1/' | awk '{s += $1} END {print s + 0}')
bad=$(cat "$OUT"/log.* | grep -a " calls, " | grep -av ": 0 differ; 0 could not be compared" | wc -l | tr -d ' ')
rm -rf "$OUT"
if [ "$bad" != 0 ] || [ "$calls" = 0 ]; then
    echo "FAILED: rewrites differ from the game's code (or nothing was compared)"
    exit 1
fi
echo "OK: $calls calls of rewritten routines compared with the game's code, none differ"
