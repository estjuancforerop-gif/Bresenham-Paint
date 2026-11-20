// ====================================================================
//  algoritmos.cpp
//  Autor: Juan Camilo Forero Paez
// ====================================================================
#include "algoritmos.h"
#include <cstdlib>    // abs
#include <utility>    // swap
#include <algorithm>  // min, max

// --------------------------------------------------------------------
// LINEA DE BRESENHAM
// la idea: en vez de calcular la pendiente con decimales, uso un numero
// entero D (el parametro de decision) que me dice si la siguiente casilla
// va en la misma fila o en la fila vecina. asi solo uso sumas y restas.
//
// como la linea se comporta distinto segun cuanto se inclina, la parto en
// dos funciones: una para lineas "mas anchas que altas" (lineaHorizontal)
// y otra para las "mas altas que anchas" (lineaVertical).
// --------------------------------------------------------------------

// esto sirve para lineas donde |dx| >= |dy| (avanzo 1 casilla en X cada vez)
static void lineaHorizontal(int x0, int y0, int x1, int y1, std::vector<Celda>& celdas)
{
    // quiero ir siempre de izquierda a derecha, si no, cambio los puntos
    if (x0 > x1)
    {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    int dx = x1 - x0;             // cuanto avanzo en X (siempre >= 0)
    int dy = y1 - y0;             // cuanto cambio en Y (puede ser negativo)

    int dir = (dy < 0) ? -1 : 1;  // si Y sube o baja
    dy = dy * dir;                // dejo dy positivo para no tener casos raros

    int D = 2 * dy - dx;          // parametro de decision inicial
    int y = y0;

    for (int x = x0; x <= x1; x++)
    {
        celdas.push_back({ x, y });   // esta casilla es parte de la linea

        if (D >= 0)
        {
            y = y + dir;              // me paso a la fila vecina
            D = D - 2 * dx;
        }
        D = D + 2 * dy;               // actualizo el parametro de decision
    }
}

// esto sirve para lineas donde |dy| > |dx|: es lo mismo pero cambiando los
// papeles de X y Y (ahora avanzo 1 casilla en Y cada vez)
static void lineaVertical(int x0, int y0, int x1, int y1, std::vector<Celda>& celdas)
{
    // quiero ir siempre de arriba hacia abajo
    if (y0 > y1)
    {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    int dx = x1 - x0;             // cuanto cambio en X (puede ser negativo)
    int dy = y1 - y0;             // cuanto avanzo en Y (siempre >= 0)

    int dir = (dx < 0) ? -1 : 1;
    dx = dx * dir;

    int D = 2 * dx - dy;
    int x = x0;

    for (int y = y0; y <= y1; y++)
    {
        celdas.push_back({ x, y });

        if (D >= 0)
        {
            x = x + dir;          // me paso a la columna vecina
            D = D - 2 * dy;
        }
        D = D + 2 * dx;
    }
}

// esto sirve para no tener que pensar cual de las dos usar: comparo que
// tan grande es el cambio en X contra el cambio en Y
void dibujarLinea(int x0, int y0, int x1, int y1, std::vector<Celda>& celdas)
{
    int dx = x1 - x0;
    int dy = y1 - y0;

    if (std::abs(dx) >= std::abs(dy))
        lineaHorizontal(x0, y0, x1, y1, celdas);
    else
        lineaVertical(x0, y0, x1, y1, celdas);
}

// --------------------------------------------------------------------
// CIRCULO CON PUNTO MEDIO
// un circulo es simetrico: si calculo UN octavo (45 grados) puedo sacar
// los otros 7 por simetria. por eso solo recorro desde arriba hasta la
// diagonal y en cada paso pinto 8 casillas.
// --------------------------------------------------------------------

// esto sirve para pintar los 8 puntos simetricos de (x, y) alrededor del centro
static void puntosSimetricos(int cx, int cy, int x, int y, std::vector<Celda>& celdas)
{
    celdas.push_back({ cx + x, cy + y });
    celdas.push_back({ cx - x, cy + y });
    celdas.push_back({ cx + x, cy - y });
    celdas.push_back({ cx - x, cy - y });
    celdas.push_back({ cx + y, cy + x });
    celdas.push_back({ cx - y, cy + x });
    celdas.push_back({ cx + y, cy - x });
    celdas.push_back({ cx - y, cy - x });
}

void dibujarCirculo(int cx, int cy, int r, std::vector<Celda>& celdas)
{
    int x = 0;
    int y = r;
    int D = 1 - r;                          // parametro de decision inicial

    puntosSimetricos(cx, cy, x, y, celdas); // el primer punto es el de arriba del circulo

    while (x < y)
    {
        x = x + 1;                          // x siempre avanza

        if (D < 0)
        {
            D = D + 2 * x + 1;              // el punto medio quedo adentro: y no cambia
        }
        else
        {
            y = y - 1;                      // el punto medio quedo afuera: y baja una casilla
            D = D + 2 * (x - y) + 1;
        }

        puntosSimetricos(cx, cy, x, y, celdas);
    }
}

// --------------------------------------------------------------------
// POLIGONO
// esto sirve para hacer triangulos, rectangulos y formas libres sin
// inventar nada nuevo: un poligono es solo una lista de puntos unidos
// de dos en dos con la linea de Bresenham que ya tengo.
// --------------------------------------------------------------------
void dibujarPoligono(const std::vector<Celda>& esquinas, bool cerrado, std::vector<Celda>& celdas)
{
    if (esquinas.empty()) return;

    // con un solo punto pinto solo esa casilla
    if (esquinas.size() == 1)
    {
        celdas.push_back(esquinas[0]);
        return;
    }

    // une cada punto con el siguiente
    for (size_t i = 0; i + 1 < esquinas.size(); i++)
        dibujarLinea(esquinas[i].first, esquinas[i].second,
                     esquinas[i + 1].first, esquinas[i + 1].second, celdas);

    // si es cerrado, une el ultimo con el primero
    if (cerrado && esquinas.size() > 2)
        dibujarLinea(esquinas.back().first, esquinas.back().second,
                     esquinas[0].first, esquinas[0].second, celdas);
}

// --------------------------------------------------------------------
// RELLENO POR INUNDACION (flood fill)
// la idea: en vez de buscar "que hay adentro" (es dificil con figuras
// feas), busco "que hay afuera". empiezo en una esquina que seguro esta
// afuera y voy llenando de vecino en vecino (arriba, abajo, izquierda,
// derecha) sin cruzar el contorno. lo que nunca se alcanza es el interior.
//
// funciona con cualquier figura cerrada, incluso las formas libres con
// huecos. (la linea de Bresenham es 8-conexa, por eso inundo con 4 vecinos:
// asi el agua no se cuela por las diagonales del contorno)
// --------------------------------------------------------------------
std::vector<Celda> rellenarContorno(const std::vector<Celda>& contorno)
{
    std::vector<Celda> interior;
    if (contorno.empty()) return interior;

    // 1) busco la caja que encierra al contorno
    int minX = contorno[0].first, maxX = contorno[0].first;
    int minY = contorno[0].second, maxY = contorno[0].second;
    for (size_t i = 0; i < contorno.size(); i++)
    {
        minX = std::min(minX, contorno[i].first);
        maxX = std::max(maxX, contorno[i].first);
        minY = std::min(minY, contorno[i].second);
        maxY = std::max(maxY, contorno[i].second);
    }

    // la caja lleva 1 casilla de margen para tener un "afuera" por donde empezar
    int ancho = maxX - minX + 3;
    int alto  = maxY - minY + 3;

    // si la figura es enorme (se salio mucho de la grilla) no la relleno
    if ((long long)ancho * alto > 4000000LL) return interior;

    // 2) mapa de la caja: 0 = vacio, 1 = contorno, 2 = afuera (ya inundado)
    std::vector<char> mapa((size_t)ancho * alto, 0);
    for (size_t i = 0; i < contorno.size(); i++)
    {
        int px = contorno[i].first  - minX + 1;
        int py = contorno[i].second - minY + 1;
        mapa[(size_t)py * ancho + px] = 1;
    }

    // 3) inundo desde la esquina (0, 0), que es margen y por eso es "afuera"
    std::vector<int> pendientes;
    pendientes.push_back(0);
    mapa[0] = 2;

    const int movX[4] = { 1, -1, 0, 0 };
    const int movY[4] = { 0, 0, 1, -1 };

    while (!pendientes.empty())
    {
        int actual = pendientes.back();
        pendientes.pop_back();
        int x = actual % ancho;
        int y = actual / ancho;

        for (int k = 0; k < 4; k++)
        {
            int nx = x + movX[k];
            int ny = y + movY[k];
            if (nx < 0 || nx >= ancho || ny < 0 || ny >= alto) continue;

            size_t vecino = (size_t)ny * ancho + nx;
            if (mapa[vecino] == 0)          // solo inundo casillas vacias
            {
                mapa[vecino] = 2;
                pendientes.push_back((int)vecino);
            }
        }
    }

    // 4) lo que quedo en 0 es el interior (el agua nunca llego)
    for (int y = 0; y < alto; y++)
        for (int x = 0; x < ancho; x++)
            if (mapa[(size_t)y * ancho + x] == 0)
                interior.push_back({ x + minX - 1, y + minY - 1 });

    return interior;
}
