"""
Tarea 4 - Implementación de la función Sphere con DEAP

Representación: Real
Dimensión: 10
Límites: [-5, 5]
Población: 50 individuos
Generaciones: 50
"""

import random
import numpy as np
import matplotlib.pyplot as plt

from deap import base, creator, tools


# ============================================================
# 1. PARÁMETROS DEL ALGORITMO
# ============================================================

DIM = 10
LOW = -5.0
UP = 5.0

POP_SIZE = 50
NGEN = 50

CXPB = 0.90       # Probabilidad de cruzamiento
ETA_C = 15.0      # Parámetro del cruzamiento SBX
ETA_M = 20.0      # Parámetro de mutación polinomial
INDPB = 1.0 / DIM # Probabilidad de mutar cada variable

SEED = 42


# ============================================================
# 2. CONTROL DE ALEATORIEDAD
# ============================================================

random.seed(SEED)
np.random.seed(SEED)


# ============================================================
# 3. DEFINICIÓN DEL FITNESS
# ============================================================

# weights=(-1.0,) indica que el problema es de minimización
if not hasattr(creator, "FitnessMin"):
    creator.create(
        "FitnessMin",
        base.Fitness,
        weights=(-1.0,)
    )

if not hasattr(creator, "Individual"):
    creator.create(
        "Individual",
        list,
        fitness=creator.FitnessMin
    )


# ============================================================
# 4. FUNCIÓN OBJETIVO SPHERE
# ============================================================

def sphere(individual):
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

    return (sum(x ** 2 for x in individual),)


# ============================================================
# 5. CONFIGURACIÓN DE DEAP
# ============================================================

toolbox = base.Toolbox()


# Representación real:
# cada variable se genera aleatoriamente entre -5 y 5
toolbox.register(
    "attr_float",
    random.uniform,
    LOW,
    UP
)


# Individuo de 10 variables reales
toolbox.register(
    "individual",
    tools.initRepeat,
    creator.Individual,
    toolbox.attr_float,
    n=DIM
)


# Población formada por individuos
toolbox.register(
    "population",
    tools.initRepeat,
    list,
    toolbox.individual
)


# Función de evaluación
toolbox.register(
    "evaluate",
    sphere
)


# Selección por torneo
toolbox.register(
    "select",
    tools.selTournament,
    tournsize=2
)


# Cruzamiento SBX
toolbox.register(
    "mate",
    tools.cxSimulatedBinaryBounded,
    eta=ETA_C,
    low=LOW,
    up=UP
)


# Mutación polinomial
toolbox.register(
    "mutate",
    tools.mutPolynomialBounded,
    eta=ETA_M,
    low=LOW,
    up=UP,
    indpb=INDPB
)


# ============================================================
# 6. CREACIÓN DE LA POBLACIÓN INICIAL
# ============================================================

population = toolbox.population(n=POP_SIZE)


# Evaluar población inicial
for individual in population:
    individual.fitness.values = toolbox.evaluate(individual)


# Listas para almacenar la evolución del fitness
best_history = []
avg_history = []


# ============================================================
# 7. ALGORITMO EVOLUTIVO
# ============================================================

print("Ejecución del algoritmo evolutivo con DEAP")
print("=" * 65)

print(f"{'Generación':<12}"
      f"{'Mejor fitness':<25}"
      f"{'Fitness promedio':<25}")

print("-" * 65)


for generation in range(1, NGEN + 1):

    offspring = []

    # --------------------------------------------------------
    # Selección, cruzamiento y mutación
    # --------------------------------------------------------

    while len(offspring) < POP_SIZE:

        # Seleccionar dos padres
        parent1, parent2 = toolbox.select(population, 2)

        # Copiar los padres
        child1 = creator.Individual(parent1)
        child2 = creator.Individual(parent2)

        # Cruzamiento
        if random.random() < CXPB:
            toolbox.mate(child1, child2)

        # Mutación
        toolbox.mutate(child1)
        toolbox.mutate(child2)

        # Evaluar descendientes
        child1.fitness.values = toolbox.evaluate(child1)
        child2.fitness.values = toolbox.evaluate(child2)

        offspring.append(child1)

        if len(offspring) < POP_SIZE:
            offspring.append(child2)


    # --------------------------------------------------------
    # REEMPLAZO ELITISTA (mu + lambda)
    # --------------------------------------------------------

    # Combinar padres y descendientes
    combined_population = population + offspring

    # Conservar los mejores 50 individuos
    population = tools.selBest(
        combined_population,
        POP_SIZE
    )


    # --------------------------------------------------------
    # ESTADÍSTICAS DE LA GENERACIÓN
    # --------------------------------------------------------

    fitness_values = [
        individual.fitness.values[0]
        for individual in population
    ]

    best_fitness = np.min(fitness_values)
    avg_fitness = np.mean(fitness_values)

    best_history.append(best_fitness)
    avg_history.append(avg_fitness)


    # Mostrar información de cada generación
    print(
        f"{generation:<12}"
        f"{best_fitness:<25.10f}"
        f"{avg_fitness:<25.10f}"
    )


# ============================================================
# 8. MEJOR SOLUCIÓN ENCONTRADA
# ============================================================

best_individual = tools.selBest(
    population,
    1
)[0]


print("\n" + "=" * 65)
print("RESULTADOS FINALES")
print("=" * 65)

print("\nMejor solución encontrada:")

for i, value in enumerate(best_individual):
    print(f"x{i + 1} = {value:.10f}")


print("\nMejor valor de fitness:")
print(f"f(x) = {best_individual.fitness.values[0]:.12f}")


# ============================================================
# 9. FITNESS PROMEDIO POR GENERACIÓN
# ============================================================

print("\nFitness promedio por generación:")
print("-" * 45)

for generation, avg in enumerate(avg_history, start=1):

    print(
        f"Generación {generation:2d}: "
        f"{avg:.10f}"
    )


# ============================================================
# 10. GRÁFICA DE CONVERGENCIA
# ============================================================

generations = range(1, NGEN + 1)

plt.figure(figsize=(10, 6))

plt.plot(
    generations,
    best_history,
    label="Mejor fitness"
)

plt.plot(
    generations,
    avg_history,
    label="Fitness promedio"
)

plt.xlabel("Generación")
plt.ylabel("Fitness")

plt.title(
    "Convergencia del algoritmo evolutivo DEAP\n"
    "Función Sphere"
)

plt.legend()

plt.grid(
    True,
    alpha=0.3
)

plt.tight_layout()

plt.show()