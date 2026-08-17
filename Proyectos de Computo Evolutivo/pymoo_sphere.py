"""
Tarea 4 - Implementación de la función Sphere con PYMOO

Dimensión: 10
Límites: [-5, 5]
Población: 50 individuos
Generaciones: 50
Algoritmo: Algoritmo Genético (GA)
"""

import time
import numpy as np
import matplotlib.pyplot as plt

from pymoo.algorithms.soo.nonconvex.ga import GA
from pymoo.core.callback import Callback
from pymoo.core.problem import ElementwiseProblem
from pymoo.operators.crossover.sbx import SBX
from pymoo.operators.mutation.pm import PM
from pymoo.operators.sampling.rnd import FloatRandomSampling
from pymoo.optimize import minimize


# ============================================================
# 1. PARÁMETROS DEL PROBLEMA
# ============================================================

DIM = 10
LOW = -5.0
UP = 5.0

POP_SIZE = 50
NGEN = 50

CXPB = 0.90       # Probabilidad de cruzamiento
ETA_C = 15.0      # Parámetro del cruzamiento SBX
ETA_M = 20.0      # Parámetro de mutación polinomial
PROB_VAR = 1.0 / DIM

SEED = 42


# ============================================================
# 2. DEFINICIÓN DEL PROBLEMA
# ============================================================

class SphereProblem(ElementwiseProblem):

    def __init__(self):

        super().__init__(
            n_var=DIM,          # Número de variables
            n_obj=1,            # Un objetivo
            n_ieq_constr=0,     # Sin restricciones de desigualdad
            n_eq_constr=0,      # Sin restricciones de igualdad
            xl=LOW,             # Límite inferior
            xu=UP               # Límite superior
        )


    def _evaluate(self, x, out, *args, **kwargs):

        """
        Función Sphere:

                 D
        f(x) =  Σ xi²
                i=1

        El mínimo global se encuentra en:

        x = [0, 0, ..., 0]

        con:

        f(x) = 0
        """

        out["F"] = np.sum(x ** 2)


# ============================================================
# 3. CALLBACK PARA REGISTRAR LA CONVERGENCIA
# ============================================================

class ConvergenceCallback(Callback):

    def __init__(self):

        super().__init__()

        self.generations = []
        self.best_history = []
        self.avg_history = []


    def notify(self, algorithm):

        # Obtener los valores de fitness de la población actual
        fitness = algorithm.pop.get("F").astype(float).reshape(-1)

        # Obtener número de generación
        generation = algorithm.n_gen

        # Calcular mejor fitness
        best_fitness = np.min(fitness)

        # Calcular fitness promedio
        avg_fitness = np.mean(fitness)

        # Guardar resultados
        self.generations.append(generation)
        self.best_history.append(best_fitness)
        self.avg_history.append(avg_fitness)


# ============================================================
# 4. CREACIÓN DEL PROBLEMA
# ============================================================

problem = SphereProblem()


# ============================================================
# 5. ALGORITMO EVOLUTIVO
# ============================================================

algorithm = GA(

    pop_size=POP_SIZE,

    # Población inicial aleatoria
    sampling=FloatRandomSampling(),

    # Cruzamiento SBX
    crossover=SBX(
        prob=CXPB,
        eta=ETA_C
    ),

    # Mutación polinomial
    mutation=PM(
        prob=1.0,
        prob_var=PROB_VAR,
        eta=ETA_M
    ),

    eliminate_duplicates=False
)


# ============================================================
# 6. CRITERIO DE TERMINACIÓN
# ============================================================

termination = (
    "n_gen",
    NGEN
)


# ============================================================
# 7. CALLBACK
# ============================================================

callback = ConvergenceCallback()


# ============================================================
# 8. EJECUCIÓN DEL PROCESO DE OPTIMIZACIÓN
# ============================================================

print("Ejecución del algoritmo evolutivo con PYMOO")
print("=" * 65)

start = time.perf_counter()


result = minimize(

    problem,

    algorithm,

    termination=termination,

    seed=SEED,

    callback=callback,

    verbose=False
)


elapsed = time.perf_counter() - start


# ============================================================
# 9. INFORMACIÓN DE CONVERGENCIA
# ============================================================

print(
    f"\n{'Generación':<12}"
    f"{'Mejor fitness':<25}"
    f"{'Fitness promedio':<25}"
)

print("-" * 65)


for generation, best, avg in zip(
    callback.generations,
    callback.best_history,
    callback.avg_history
):

    print(
        f"{generation:<12}"
        f"{best:<25.10f}"
        f"{avg:<25.10f}"
    )


# ============================================================
# 10. EXTRACCIÓN DE RESULTADOS
# ============================================================

best_solution = np.array(
    result.X,
    dtype=float
).reshape(-1)

best_fitness = float(
    np.array(
        result.F,
        dtype=float
    ).reshape(-1)[0]
)


# ============================================================
# 11. MOSTRAR MEJOR SOLUCIÓN ENCONTRADA
# ============================================================

print("\n" + "=" * 65)
print("RESULTADOS FINALES")
print("=" * 65)


print("\nMejor solución encontrada:")


for i, value in enumerate(best_solution):

    print(
        f"x{i + 1} = {value:.10f}"
    )


# ============================================================
# 12. MOSTRAR MEJOR VALOR DE FITNESS
# ============================================================

print("\nMejor valor de fitness:")

print(
    f"f(x) = {best_fitness:.12f}"
)


# ============================================================
# 13. MOSTRAR TIEMPO DE EJECUCIÓN
# ============================================================

print("\nTiempo de ejecución:")

print(
    f"{elapsed:.6f} segundos"
)


# ============================================================
# 14. RESUMEN DE CONVERGENCIA
# ============================================================

print("\nResumen de convergencia:")

print(
    f"Fitness inicial: "
    f"{callback.best_history[0]:.10f}"
)

print(
    f"Fitness final: "
    f"{callback.best_history[-1]:.10f}"
)

print(
    f"Fitness promedio final: "
    f"{callback.avg_history[-1]:.10f}"
)


# ============================================================
# 15. GRÁFICA DE CONVERGENCIA
# ============================================================

plt.figure(
    figsize=(10, 6)
)


plt.plot(
    callback.generations,
    callback.best_history,
    label="Mejor fitness"
)


plt.plot(
    callback.generations,
    callback.avg_history,
    label="Fitness promedio"
)


plt.xlabel(
    "Generación"
)


plt.ylabel(
    "Fitness"
)


plt.title(
    "Convergencia del algoritmo genético PYMOO\n"
    "Función Sphere"
)


plt.legend()


plt.grid(
    True,
    alpha=0.3
)


plt.tight_layout()


plt.show()