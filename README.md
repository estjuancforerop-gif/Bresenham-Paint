# PAINT — Dibujo sobre grilla

**Nombre:** Juan Camilo Forero Páez  
**Código estudiantil:** _(agregar mi código institucional antes de entregar)_  
**Proyecto:** Paint
**Proyecto:** Paint sobre una grilla de casillas, desarrollado en C++ con OpenGL, GLFW, GLAD y Dear ImGui.

---

## 1. Descripción general

Este proyecto es un Paint sencillo donde todo se dibuja sobre una **grilla de casillas cuadradas**.
Cada figura se calcula con los algoritmos vistos en clase (Bresenham para la línea, punto medio para
el círculo), y los demás elementos (cuadrado, triángulo, forma libre, relleno y transformaciones) los armé
**encima de esos mismos algoritmos**, con una implementación organizada en módulos para poder explicar y modificar cada parte.

La idea principal es esta: los algoritmos **no dibujan**, solo calculan *qué casillas hay que pintar*.
Después otra parte del programa las pinta. Así puedo cambiar la forma de pintar sin tocar los algoritmos.

## 2. Lo que hace el programa (requisitos del parcial)

| Requisito | Dónde está |
|---|---|
| Línea de Bresenham | `algoritmos.cpp` → `dibujarLinea` (usa `lineaHorizontal` y `lineaVertical`) |
| Círculo de Bresenham (punto medio) | `algoritmos.cpp` → `dibujarCirculo` |
| Cuadrados y rectángulos | `figuras.cpp` → `crearFigura` (4 esquinas unidas con líneas de Bresenham) |
| Triángulos en la grilla | herramienta triángulo (3 clicks), se dibuja con `dibujarPoligono` |
| Formas libres | herramienta forma libre (polígono con los puntos que yo quiera) |
| Relleno de primitivas | `algoritmos.cpp` → `rellenarContorno` (relleno por inundación) |
| Transformaciones lineales (rotación) | `figuras.cpp` → `aplicarMatriz`, `rotarFigura` (y también `escalarFigura`) |
| Interacción con interfaz (Dear ImGui) | `interfaz.cpp` → barra de herramientas y lectura del mouse |

## 3. Cómo se usa

La barra de arriba tiene 8 botones (solo íconos). De izquierda a derecha:

| Botón | Qué hace | Cómo se usa |
|---|---|---|
| Línea | línea de Bresenham | 2 clicks: inicio y final |
| Cuadrado | cuadrado o rectángulo | 2 clicks en esquinas opuestas. Con **Shift** en el 2.º click sale cuadrado perfecto |
| Círculo | círculo con punto medio | 1.º click = centro, 2.º click = un punto del borde (define el radio) |
| Triángulo | triángulo | 3 clicks, uno por cada vértice |
| Forma libre | polígono libre | un click por vértice. Termina con **click derecho** o **Enter** |
| Relleno | rellena o vacía una figura | click sobre la figura |
| Rotar | gira la figura 15° sobre su centro | click sobre la figura (**Shift** gira al otro lado) |
| Escalar | agranda la figura ×1.25 | click sobre la figura (**Shift** la achica) |

**Atajos:** `Ctrl+Z` deshace (se puede varias veces), `Supr` borra todo, `Esc` cancela la figura que estoy haciendo.

Mientras hago una figura, las casillas que ya clickeé quedan marcadas. La casilla donde está el mouse
tiene un borde blanco para saber dónde voy a hacer click. La ventana se puede agrandar: en vez de estirar
las casillas, aparecen **más casillas**. Por consola se imprime, paso a paso, cada casilla que calculó el
algoritmo de la figura que acabo de terminar.

## 4. Arquitectura del proyecto

El programa está partido en módulos. Cada uno hace una sola cosa:

```
                     +-----------------+
                     |    main.cpp     |   ventana, shaders, buffers y bucle principal
                     +--------+--------+
                              |
          +-------------------+-------------------+
          |                                       |
+---------v----------+                 +----------v---------+
|    interfaz.cpp    |                 |     figuras.cpp    |
| barra (Dear ImGui) |  ------------>  | lista de figuras,  |
| mouse y teclado    |   crea / pide   | relleno, rotar,    |
+--------------------+   cambios       | escalar, deshacer  |
                                       +----------+---------+
                                                  |
                          +-----------------------+---------------------+
                          |                                             |
                +---------v----------+                       +----------v---------+
                |   algoritmos.cpp   |                       |     grilla.cpp     |
                | Bresenham, punto   |   devuelve casillas   | casillas, vertices |
                | medio, polígono,   |  ------------------>  | y colores          |
                | relleno            |                       +--------------------+
                +--------------------+
```

**Qué hace cada módulo**

- **`grilla.cpp / .h`**: guarda la grilla. Cada casilla son 4 vértices (2 triángulos) con su color.
  Pintar una casilla es cambiar el color de sus 4 vértices. No sabe nada de figuras.
- **`algoritmos.cpp / .h`**: los algoritmos de clase. Reciben puntos y devuelven una lista de casillas.
  No dibujan nada.
- **`figuras.cpp / .h`**: guarda la lista de figuras (`figuras`) y la figura que estoy haciendo (`enCurso`).
  También repinta la grilla, rellena, aplica transformaciones y maneja el historial para deshacer.
- **`interfaz.cpp / .h`**: la barra de herramientas con Dear ImGui y la lógica de los clicks. Convierte la
  posición del mouse en una casilla.
- **`main.cpp`**: crea la ventana, compila los shaders, sube la grilla a la GPU y repite el bucle.

**Qué pasa en cada cuadro** (`dibujarCuadro` en `main.cpp`):

1. Empiezo el cuadro de ImGui y calculo dónde está el lienzo (la zona de la grilla).
2. Reviso si la ventana cambió de tamaño; si cambió, recalculo cuántas casillas caben.
3. Leo el mouse y el teclado (`procesarMouse`, `procesarTeclado`) y armo la barra (`dibujarBarra`).
4. Si algo cambió (`grillaCambio`), repinto: limpio la grilla y pinto las figuras en orden
   (para cada una: primero el relleno y luego el contorno encima).
5. Dibujo la grilla con OpenGL y encima la interfaz.

**Decisión importante:** la grilla *no* guarda las figuras, solo colores. Las figuras viven en una lista
y cada vez que algo cambia se repinta todo desde cero. Gracias a eso, rotar, rellenar, borrar y deshacer
son solo cambiar la lista y volver a pintar.

## 5. Algoritmos (explicados para mí)

**Línea de Bresenham.** En vez de calcular la pendiente con decimales, uso un número entero `D`
(parámetro de decisión) que me dice si la siguiente casilla va en la misma fila o en la vecina. Solo hay
sumas y restas. Tengo dos versiones: `lineaHorizontal` (cuando avanza más en X que en Y) y `lineaVertical`
(al revés), y `dibujarLinea` decide cuál usar comparando `|dx|` con `|dy|`. Así funciona en los 8 octantes.

**Círculo (punto medio).** Un círculo es simétrico, entonces solo calculo un octavo y los otros 7 salen por
simetría (`puntosSimetricos`). En cada paso el parámetro `D` me dice si el punto medio quedó dentro o fuera
del círculo y con eso decido si `y` baja o no.

**Polígonos (rectángulo, triángulo, forma libre).** Un polígono es una lista de puntos unidos de dos en dos
con la línea de Bresenham (`dibujarPoligono`). El rectángulo son 4 esquinas, el triángulo 3 y la forma libre
los que yo quiera.

**Relleno por inundación.** En vez de buscar "lo de adentro", busco "lo de afuera": empiezo desde una esquina
que seguro está afuera y voy llenando vecino por vecino sin cruzar el contorno. Lo que nunca se alcanza es el
interior. Funciona con cualquier figura cerrada, incluso formas libres cóncavas. Inundo con 4 vecinos porque
la línea de Bresenham es 8-conexa, y así el "agua" no se cuela por las diagonales.

**Transformaciones lineales.** Rotar y escalar son multiplicar cada punto por una matriz 2×2:

```
| x' |   | a  b |   | x |          rotación:  | cos  -sin |      escala:  | s  0 |
| y' | = | c  d | * | y |                     | sin   cos |               | 0  s |
```

Esa multiplicación gira alrededor de (0,0), que es la esquina de la grilla. Para que la figura gire sobre su
**propio centro**: (1) le resto el centro a cada punto, (2) multiplico por la matriz y (3) vuelvo a sumar el
centro (`aplicarMatriz`). Guardo los puntos con decimales para que, al rotar varias veces, no se acumulen
errores de redondeo; solo redondeo a casilla al momento de pintar.

## 6. Estructura de archivos

```
Paint/
├── main.cpp            ventana, shaders y bucle principal
├── grilla.cpp / .h     casillas, vértices y colores
├── algoritmos.cpp / .h Bresenham, punto medio, polígono y relleno
├── figuras.cpp / .h    lista de figuras, relleno, transformaciones, deshacer
├── interfaz.cpp / .h   barra de herramientas (Dear ImGui) y mouse
├── glad.c              carga de funciones de OpenGL
├── librerias/
│   ├── include/        cabeceras de GLFW y glad
│   ├── lib/            glfw3.lib
│   └── imgui/          Dear ImGui y sus backends para GLFW y OpenGL3
├── Paint.slnx / .vcxproj / .vcxproj.filters
└── README.md
```

## 7. Cómo compilarlo

1. Abrir `Paint.slnx` con Visual Studio 2022 (17.10 o superior).
2. Elegir la configuración **x64** (Debug o Release).
3. Compilar y ejecutar (`F5`).

Las librerías ya vienen dentro de la carpeta `librerias`, no hay que instalar nada más.

## 8. Librerías usadas

- [GLFW](https://www.glfw.org/): ventana, mouse y teclado.
- [glad](https://glad.dav1d.de/): carga de las funciones de OpenGL.
- [Dear ImGui](https://github.com/ocornut/imgui): la barra de herramientas.


## 9. Datos que debo completar antes de entregar

- Escribir mi código estudiantil en la portada de este README.
- Abrir la solución en Visual Studio y compilar en **x64**.
- Probar cada herramienta y poder explicar con mis palabras cómo funciona.
- Personalizar la interfaz, colores o controles según mis decisiones y conservar únicamente lo que pueda sustentar en la revisión.

> Nota: este README describe la organización y el funcionamiento del código incluido. La compilación final debe verificarse en el equipo donde se va a presentar, porque depende de que Visual Studio tenga instalado el compilador C++ y el SDK de Windows.
