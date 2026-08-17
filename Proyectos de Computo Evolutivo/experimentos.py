"""
Tarea 4 - Secciones 6, 8 y 9
Comparación experimental DEAP vs PYMOO

Ejecuta:
1) 30 corridas independientes por framework con N=50 y G=50.
2) Experimento automático variando N en {20, 50, 100}.
3) Genera tablas CSV y gráficas de convergencia.

Requisitos:
pip install deap pymoo numpy pandas matplotlib
"""

import random
import time
from pathlib import Path

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

from deap import base, creator, tools

from pymoo.algorithms.soo.nonconvex.ga import GA
from pymoo.core.callback import Callback
from pymoo.core.problem import ElementwiseProblem
from pymoo.operators.crossover.sbx import SBX
from pymoo.operators.mutation.pm import PM
from pymoo.operators.sampling.rnd import FloatRandomSampling
from pymoo.optimize import minimize


DIM = 10
LOW = -5.0
UP = 5.0
NGEN = 50
CXPB = 0.90
ETA_C = 15.0
ETA_M = 20.0
PROB_VAR = 1.0 / DIM

SEEDS = list(range(30))
POP_BASE = 50
POP_EXPERIMENT = [20, 50, 100]

OUTPUT = Path("resultados_tarea4")
OUTPUT.mkdir(exist_ok=True)


# ============================================================
# DEAP
# ============================================================

if not hasattr(creator, "FitnessMinT4"):
    creator.create("FitnessMinT4", base.Fitness, weights=(-1.0,))

if not hasattr(creator, "IndividualT4"):
    creator.create("IndividualT4", list, fitness=creator.FitnessMinT4)


def sphere_deap(individual):
    return (sum(x ** 2 for x in individual),)


def build_deap_toolbox():
    toolbox = base.Toolbox()

    toolbox.register("attr_float", random.uniform, LOW, UP)

    toolbox.register(
        "individual",
        tools.initRepeat,
        creator.IndividualT4,
        toolbox.attr_float,
        n=DIM
    )

    toolbox.register(
        "population",
        tools.initRepeat,
        list,
        toolbox.individual
    )

    toolbox.register("evaluate", sphere_deap)
    toolbox.register("select", tools.selTournament, tournsize=2)

    toolbox.register(
        "mate",
        tools.cxSimulatedBinaryBounded,
        eta=ETA_C,
        low=LOW,
        up=UP
    )

    toolbox.register(
        "mutate",
        tools.mutPolynomialBounded,
        eta=ETA_M,
        low=LOW,
        up=UP,
        indpb=PROB_VAR
    )

    return toolbox


def run_deap(seed, pop_size=50, ngen=50):
    random.seed(seed)
    np.random.seed(seed)

    toolbox = build_deap_toolbox()

    start = time.perf_counter()

    population = toolbox.population(n=pop_size)

    for ind in population:
        ind.fitness.values = toolbox.evaluate(ind)

    best_history = []
    avg_history = []

    for _ in range(ngen):
        offspring = []

        while len(offspring) < pop_size:
            parent1, parent2 = toolbox.select(population, 2)

            child1 = creator.IndividualT4(parent1)
            child2 = creator.IndividualT4(parent2)

            if random.random() < CXPB:
                toolbox.mate(child1, child2)

            toolbox.mutate(child1)
            toolbox.mutate(child2)

            child1.fitness.values = toolbox.evaluate(child1)
            child2.fitness.values = toolbox.evaluate(child2)

            offspring.append(child1)

            if len(offspring) < pop_size:
                offspring.append(child2)

        population = tools.selBest(
            population + offspring,
            pop_size
        )

        fitness = np.array(
            [ind.fitness.values[0] for ind in population],
            dtype=float
        )

        best_history.append(float(np.min(fitness)))
        avg_history.append(float(np.mean(fitness)))

    elapsed = time.perf_counter() - start
    best = tools.selBest(population, 1)[0]

    return {
        "framework": "DEAP",
        "seed": seed,
        "best_f": float(best.fitness.values[0]),
        "time_s": elapsed,
        "best_history": np.array(best_history, dtype=float),
        "avg_history": np.array(avg_history, dtype=float)
    }


# ============================================================
# PYMOO
# ============================================================

class SphereProblem(ElementwiseProblem):

    def __init__(self):
        super().__init__(
            n_var=DIM,
            n_obj=1,
            n_ieq_constr=0,
            n_eq_constr=0,
            xl=LOW,
            xu=UP
        )

    def _evaluate(self, x, out, *args, **kwargs):
        out["F"] = np.sum(x ** 2)


class ConvergenceCallback(Callback):

    def __init__(self):
        super().__init__()
        self.best_history = []
        self.avg_history = []

    def notify(self, algorithm):
        fitness = algorithm.pop.get("F").astype(float).reshape(-1)

        self.best_history.append(
            float(np.min(fitness))
        )

        self.avg_history.append(
            float(np.mean(fitness))
        )


def run_pymoo(seed, pop_size=50, ngen=50):
    problem = SphereProblem()
    callback = ConvergenceCallback()

    algorithm = GA(
        pop_size=pop_size,
        sampling=FloatRandomSampling(),

        crossover=SBX(
            prob=CXPB,
            eta=ETA_C
        ),

        mutation=PM(
            prob=1.0,
            prob_var=PROB_VAR,
            eta=ETA_M
        ),

        eliminate_duplicates=False
    )

    start = time.perf_counter()

    result = minimize(
        problem,
        algorithm,
        termination=("n_gen", ngen),
        seed=seed,
        callback=callback,
        verbose=False
    )

    elapsed = time.perf_counter() - start

    return {
        "framework": "PYMOO",
        "seed": seed,
        "best_f": float(np.asarray(result.F).reshape(-1)[0]),
        "time_s": elapsed,
        "best_history": np.asarray(callback.best_history, dtype=float),
        "avg_history": np.asarray(callback.avg_history, dtype=float)
    }


# ============================================================
# SECCIÓN 6 - 30 CORRIDAS
# ============================================================

def section6():
    rows = []
    histories = {
        "DEAP": [],
        "PYMOO": []
    }

    print("\nSECCIÓN 6 - 30 CORRIDAS INDEPENDIENTES")
    print("=" * 70)

    for seed in SEEDS:
        deap_result = run_deap(
            seed=seed,
            pop_size=POP_BASE,
            ngen=NGEN
        )

        pymoo_result = run_pymoo(
            seed=seed,
            pop_size=POP_BASE,
            ngen=NGEN
        )

        for result in [deap_result, pymoo_result]:
            rows.append({
                "Framework": result["framework"],
                "Semilla": result["seed"],
                "Fitness_final": result["best_f"],
                "Tiempo_s": result["time_s"]
            })

            histories[result["framework"]].append(
                result["best_history"]
            )

        print(
            f"Semilla {seed:02d} | "
            f"DEAP={deap_result['best_f']:.8e} | "
            f"PYMOO={pymoo_result['best_f']:.8e}"
        )

    df = pd.DataFrame(rows)

    summary = (
        df.groupby("Framework", as_index=False)
        .agg(
            Mejor=("Fitness_final", "min"),
            Peor=("Fitness_final", "max"),
            Promedio=("Fitness_final", "mean"),
            Desv_Est=("Fitness_final", "std"),
            Tiempo_promedio_s=("Tiempo_s", "mean")
        )
    )

    df.to_csv(
        OUTPUT / "corridas_30.csv",
        index=False
    )

    summary.to_csv(
        OUTPUT / "tabla_comparativa_seccion6.csv",
        index=False
    )

    print("\nTABLA COMPARATIVA")
    print(summary.to_string(index=False))

    return df, summary, histories


# ============================================================
# SECCIÓN 8 - VARIAR TAMAÑO DE POBLACIÓN
# ============================================================

def section8():
    rows = []

    print("\nSECCIÓN 8 - EXPERIMENTO DE TAMAÑO DE POBLACIÓN")
    print("=" * 70)

    for pop_size in POP_EXPERIMENT:

        for seed in SEEDS:
            deap_result = run_deap(
                seed=seed,
                pop_size=pop_size,
                ngen=NGEN
            )

            pymoo_result = run_pymoo(
                seed=seed,
                pop_size=pop_size,
                ngen=NGEN
            )

            for result in [deap_result, pymoo_result]:
                rows.append({
                    "Framework": result["framework"],
                    "Poblacion": pop_size,
                    "Semilla": seed,
                    "Fitness_final": result["best_f"],
                    "Tiempo_s": result["time_s"]
                })

        print(
            f"Población N={pop_size}: "
            "30 corridas por framework completadas"
        )

    df = pd.DataFrame(rows)

    summary = (
        df.groupby(
            ["Framework", "Poblacion"],
            as_index=False
        )
        .agg(
            Promedio_mejor_fitness=("Fitness_final", "mean"),
            Desv_Est=("Fitness_final", "std"),
            Tiempo_promedio_s=("Tiempo_s", "mean")
        )
    )

    df.to_csv(
        OUTPUT / "experimento_poblacion_corridas.csv",
        index=False
    )

    summary.to_csv(
        OUTPUT / "tabla_experimento_poblacion.csv",
        index=False
    )

    print("\nRESULTADOS POR TAMAÑO DE POBLACIÓN")
    print(summary.to_string(index=False))

    return df, summary


# ============================================================
# SECCIÓN 9 - CONVERGENCIA PROMEDIO DE 30 CORRIDAS
# ============================================================

def section9(histories):
    mean_curves = {}

    for framework in ["DEAP", "PYMOO"]:
        matrix = np.vstack(histories[framework])
        mean_curves[framework] = np.mean(matrix, axis=0)

        plt.figure(figsize=(9, 5))

        plt.plot(
            np.arange(1, NGEN + 1),
            mean_curves[framework],
            label=framework
        )

        plt.yscale("log")
        plt.xlabel("Generación")
        plt.ylabel("Mejor fitness promedio")
        plt.title(
            f"Convergencia promedio de {framework} - función Sphere"
        )
        plt.legend()
        plt.grid(True, alpha=0.3)
        plt.tight_layout()

        plt.savefig(
            OUTPUT / f"convergencia_{framework.lower()}.png",
            dpi=300
        )

        plt.close()

    plt.figure(figsize=(9, 5))

    for framework in ["DEAP", "PYMOO"]:
        plt.plot(
            np.arange(1, NGEN + 1),
            mean_curves[framework],
            label=framework
        )

    plt.yscale("log")
    plt.xlabel("Generación")
    plt.ylabel("Mejor fitness promedio")
    plt.title(
        "Comparación de convergencia promedio: DEAP vs PYMOO"
    )
    plt.legend()
    plt.grid(True, alpha=0.3)
    plt.tight_layout()

    plt.savefig(
        OUTPUT / "convergencia_comparada.png",
        dpi=300
    )

    plt.close()

    print(
        "\nGráficas de convergencia guardadas en:",
        OUTPUT.resolve()
    )


if __name__ == "__main__":

    _, summary6, histories = section6()

    _, summary8 = section8()

    section9(histories)

    print("\nProceso terminado.")
    print(
        "Revise la carpeta:",
        OUTPUT.resolve()
    )
