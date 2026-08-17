import random
import matplotlib.pyplot as plt

# ============================================================
# Parámetros del algoritmo
# ============================================================

L = 10
tamano_poblacion = 30
generaciones = 30
probabilidad_cruza = 0.8

# Puedes cambiar estos métodos
metodo_seleccion = "torneo"     # "ruleta" o "torneo"
metodo_cruza = "uniforme"       # "un_punto" o "uniforme"


# ============================================================
# Representación binaria
# ============================================================

def crear_individuo():
    individuo = []

    for i in range(L):
        bit = random.randint(0, 1)
        individuo.append(bit)

    return individuo


def crear_poblacion():
    poblacion = []

    for i in range(tamano_poblacion):
        individuo = crear_individuo()
        poblacion.append(individuo)

    return poblacion


# ============================================================
# Función objetivo
# ============================================================

def funcion_objetivo(individuo):
    return sum(individuo)


# ============================================================
# Función de fitness
# En este problema puede ser igual a la función objetivo
# ============================================================

def fitness(individuo):
    return funcion_objetivo(individuo)


# ============================================================
# Selección por ruleta
# ============================================================

def seleccion_ruleta(poblacion):
    suma_fitness = 0

    for individuo in poblacion:
        suma_fitness += fitness(individuo)

    # Si todos tienen fitness 0, se selecciona uno aleatorio
    if suma_fitness == 0:
        return random.choice(poblacion)

    numero = random.uniform(0, suma_fitness)
    acumulado = 0

    for individuo in poblacion:
        acumulado += fitness(individuo)

        if acumulado >= numero:
            return individuo

    return poblacion[-1]


# ============================================================
# Selección por torneo
# ============================================================

def seleccion_torneo(poblacion, tamano_torneo=3):
    competidores = random.sample(poblacion, tamano_torneo)

    mejor = competidores[0]

    for individuo in competidores:
        if fitness(individuo) > fitness(mejor):
            mejor = individuo

    return mejor


# ============================================================
# Función general de selección
# ============================================================

def seleccionar_padre(poblacion):
    if metodo_seleccion == "ruleta":
        return seleccion_ruleta(poblacion)

    elif metodo_seleccion == "torneo":
        return seleccion_torneo(poblacion)

    else:
        raise ValueError("Método de selección no válido")


# ============================================================
# Cruzamiento de un punto
# ============================================================

def cruza_un_punto(padre1, padre2):
    punto = random.randint(1, L - 1)

    hijo1 = padre1[:punto] + padre2[punto:]
    hijo2 = padre2[:punto] + padre1[punto:]

    return hijo1, hijo2


# ============================================================
# Cruzamiento uniforme
# ============================================================

def cruza_uniforme(padre1, padre2):
    hijo1 = []
    hijo2 = []

    for i in range(L):
        if random.random() < 0.5:
            hijo1.append(padre1[i])
            hijo2.append(padre2[i])
        else:
            hijo1.append(padre2[i])
            hijo2.append(padre1[i])

    return hijo1, hijo2


# ============================================================
# Función general de cruzamiento
# ============================================================

def cruzar(padre1, padre2):
    if random.random() <= probabilidad_cruza:

        if metodo_cruza == "un_punto":
            return cruza_un_punto(padre1, padre2)

        elif metodo_cruza == "uniforme":
            return cruza_uniforme(padre1, padre2)

        else:
            raise ValueError("Método de cruza no válido")

    else:
        # Si no ocurre cruzamiento, los hijos son copias de los padres
        return padre1[:], padre2[:]


# ============================================================
# Obtener mejor individuo
# ============================================================

def obtener_mejor(poblacion):
    mejor = poblacion[0]

    for individuo in poblacion:
        if fitness(individuo) > fitness(mejor):
            mejor = individuo

    return mejor


# ============================================================
# Calcular promedio de fitness
# ============================================================

def promedio_fitness(poblacion):
    suma = 0

    for individuo in poblacion:
        suma += fitness(individuo)

    promedio = suma / len(poblacion)

    return promedio


# ============================================================
# Algoritmo evolutivo
# ============================================================

poblacion = crear_poblacion()

mejores_por_generacion = []
promedios_por_generacion = []

print("Población inicial:")
for individuo in poblacion:
    print(individuo, "fitness =", fitness(individuo))

print("\nMétodo de selección:", metodo_seleccion)
print("Método de cruza:", metodo_cruza)
print("Probabilidad de cruza:", probabilidad_cruza)

for generacion in range(1, generaciones + 1):

    nueva_poblacion = []

    # Reemplazo generacional completo:
    # Toda la población se reemplaza por nuevos hijos
    while len(nueva_poblacion) < tamano_poblacion:

        padre1 = seleccionar_padre(poblacion)
        padre2 = seleccionar_padre(poblacion)

        hijo1, hijo2 = cruzar(padre1, padre2)

        nueva_poblacion.append(hijo1)

        if len(nueva_poblacion) < tamano_poblacion:
            nueva_poblacion.append(hijo2)

    poblacion = nueva_poblacion

    mejor = obtener_mejor(poblacion)
    promedio = promedio_fitness(poblacion)

    mejores_por_generacion.append(fitness(mejor))
    promedios_por_generacion.append(promedio)

    print("\nGeneración:", generacion)
    print("Mejor individuo:", mejor)
    print("Fitness del mejor:", fitness(mejor))
    print("Promedio de fitness:", promedio)

    if fitness(mejor) == L:
        print("Se encontró la solución óptima.")
        # No se usa break para cumplir las 30 generaciones
        # break


# ============================================================
# Resultado final
# ============================================================

mejor_final = obtener_mejor(poblacion)

print("\n====================================")
print("Resultado final")
print("====================================")
print("Mejor individuo encontrado:", mejor_final)
print("Fitness:", fitness(mejor_final))
print("Valor máximo posible:", L)


# ============================================================
# Gráfica de convergencia
# ============================================================

plt.figure(figsize=(8, 5))
plt.plot(mejores_por_generacion, label="Mejor fitness")
plt.plot(promedios_por_generacion, label="Promedio fitness")
plt.xlabel("Generación")
plt.ylabel("Fitness")
plt.title("Convergencia del algoritmo evolutivo")
plt.legend()
plt.grid(True)
plt.show()