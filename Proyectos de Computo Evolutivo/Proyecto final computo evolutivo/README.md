# Localización de centros de acopio mediante Computación Evolutiva

## Descripción

Este proyecto aborda un problema de localización de centros de acopio de
sorgo grano en Tamaulipas, México. Se consideran los 43 municipios como
puntos de demanda y ubicaciones candidatas para instalar centros de
acopio.

El estudio utiliza información de producción agrícola municipal y
coordenadas geográficas. Las distancias entre cabeceras municipales se
calculan mediante la fórmula de Haversine. Se implementan líneas base,
un algoritmo genético (GA) para optimización monoobjetivo y NSGA-II para
optimización multiobjetivo.

## Objetivos de optimización

Se analizan tres criterios:

-   **f1:** minimizar la distancia promedio ponderada por la producción
    de sorgo.
-   **f2:** minimizar la distancia máxima entre un municipio y su centro
    de acopio más cercano.
-   **f3:** minimizar el número de centros instalados.

Para el problema monoobjetivo se estudian configuraciones con **p = 3,
5, 7 y 10 centros**. Posteriormente se analiza el compromiso entre
eficiencia y cobertura mediante NSGA-II y se permite que el número de
centros varíe entre 3 y 10.

## Métodos implementados

El notebook incluye:

-   Carga y preparación de datos.
-   Cálculo de la matriz de distancias mediante Haversine.
-   Selección aleatoria.
-   Heurística basada en producción.
-   Heurística voraz.
-   Algoritmo genético monoobjetivo.
-   30 corridas independientes para el GA.
-   Análisis estadístico mediante Wilcoxon.
-   NSGA-II para dos objetivos.
-   NSGA-II para tres objetivos.
-   Obtención de frentes de Pareto.
-   Generación de tablas, gráficas y mapas.

## Estructura recomendada

``` text
Tarea_Localizacion_Centros/
├── README.md
├── Reporte/
│   └── Localizacion_Centros_Acopio.pdf
├── Codigo/
│   └── localizacion_centros_acopio.ipynb
├── Datos/
│   ├── datos_produccion.csv
│   └── datos_coordenadas.csv
└── Resultados/
    ├── figuras/
    └── csv/
```

Los nombres de los archivos de datos pueden variar de acuerdo con los
recursos originales proporcionados para la actividad.

## Requisitos

El proyecto fue desarrollado en Python mediante Jupyter Notebook. Las
principales bibliotecas utilizadas son:

-   NumPy
-   pandas
-   Matplotlib
-   SciPy
-   PYMOO

Instalación básica:

``` bash
pip install numpy pandas matplotlib scipy pymoo
```

## Ejecución

1.  Colocar los archivos CSV requeridos en la carpeta `Datos`.
2.  Abrir el notebook ubicado en `Codigo`.
3.  Verificar las rutas de los archivos de entrada.
4.  Ejecutar las celdas del notebook en orden.
5.  Revisar los resultados, frentes de Pareto y mapas generados.

El notebook utiliza semillas controladas en los experimentos
estocásticos para facilitar la reproducibilidad.

## Resultados principales

El algoritmo genético obtuvo distancias promedio ponderadas de **32.56,
16.62, 9.64 y 3.72 km** para 3, 5, 7 y 10 centros, respectivamente.

Respecto a la selección aleatoria, las reducciones fueron
aproximadamente **72.91 %, 78.81 %, 86.97 % y 92.36 %**. La heurística
voraz resultó competitiva y alcanzó los mismos valores que el GA para 5,
7 y 10 centros.

El análisis multiobjetivo mostró que minimizar únicamente la distancia
promedio ponderada no garantiza una cobertura territorial uniforme.
NSGA-II permitió obtener alternativas de compromiso entre eficiencia
promedio y distancia máxima.

Al incorporar el número de centros como tercer objetivo, el mejor valor
de la distancia máxima alcanzó **64.31 km con nueve centros** y no se
redujo al utilizar diez centros, aunque la distancia promedio ponderada
continuó mejorando.

## Consideraciones

Las distancias corresponden a distancias geográficas entre cabeceras
municipales calculadas mediante Haversine. Los resultados representan
una aproximación para el análisis de localización y no una red logística
definitiva basada en rutas carreteras.

Como trabajo futuro pueden incorporarse distancias y tiempos por
carretera, costos de instalación y operación, capacidad de
almacenamiento y restricciones logísticas adicionales.

## Autor

**Gustavo Echeverría Salinas**\
Maestría en Ciencias e Ingeniería de Datos\
Facultad de Ingeniería y Ciencias\
Universidad Autónoma de Tamaulipas

## Asignatura

**Cómputo Evolutivo**
