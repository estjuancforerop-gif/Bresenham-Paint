// ====================================================================
//  figuras.cpp
//  Autor: Juan Camilo Forero Paez
// ====================================================================
#include "figuras.h"
#include "algoritmos.h"
#include <cmath>
#include <cstdio>
#include <iostream>
#include <algorithm>

std::vector<Figura> figuras;
FiguraEnCurso enCurso = { false, FIGURA_LINEA, { 0.0f, 0.0f, 0.0f }, {} };
bool grillaCambio = false;

static const double PI = 3.14159265358979323846;

// --------------------------------------------------------------------
// DESHACER
// esto sirve para poder deshacer cualquier cosa (dibujar, rellenar, rotar,
// borrar todo): antes de cambiar algo guardo una copia de TODA la lista, y
// deshacer solo vuelve a la ultima copia.
// --------------------------------------------------------------------
static std::vector<std::vector<Figura>> historial;
static const size_t MAXIMO_HISTORIAL = 100;

static void guardarParaDeshacer()
{
    historial.push_back(figuras);
    if (historial.size() > MAXIMO_HISTORIAL)
        historial.erase(historial.begin());
}

void deshacer()
{
    if (historial.empty()) return;
    figuras = historial.back();
    historial.pop_back();
    grillaCambio = true;
}

void borrarTodo()
{
    if (figuras.empty()) return;
    guardarParaDeshacer();
    figuras.clear();
    grillaCambio = true;
}

// --------------------------------------------------------------------
// CONVERTIR ENTRE PUNTOS Y CASILLAS
// --------------------------------------------------------------------

// esto sirve para pasar un punto con decimales a la casilla mas cercana
static Celda aCelda(const Punto& p)
{
    return Celda((int)std::lround(p.x), (int)std::lround(p.y));
}

static std::vector<Celda> casillasDeLosPuntos(const Figura& f)
{
    std::vector<Celda> lista;
    for (size_t i = 0; i < f.puntos.size(); i++)
        lista.push_back(aCelda(f.puntos[i]));
    return lista;
}

// --------------------------------------------------------------------
// CREAR UNA FIGURA A PARTIR DE LOS CLICKS
// esto sirve para convertir los clicks (casillas) en una figura con
// puntos. cada tipo usa los clicks de forma distinta.
// --------------------------------------------------------------------
Figura crearFigura(TipoFigura tipo, const std::vector<Celda>& clicks, Color color)
{
    Figura f;
    f.tipo    = tipo;
    f.radio   = 0.0;
    f.relleno = false;
    f.color   = color;
    f.colorRelleno = { color.r * 0.5f, color.g * 0.5f, color.b * 0.5f };

    switch (tipo)
    {
    case FIGURA_LINEA:
    case FIGURA_TRIANGULO:
    case FIGURA_LIBRE:
        // los puntos son los mismos clicks
        for (size_t i = 0; i < clicks.size(); i++)
            f.puntos.push_back({ (double)clicks[i].first, (double)clicks[i].second });
        break;

    case FIGURA_RECTANGULO:
    {
        // los 2 clicks son esquinas opuestas: de ahi saco las 4 esquinas.
        // guardo las 4 (y no solo 2) para que despues se pueda rotar
        int xMin = std::min(clicks[0].first,  clicks[1].first);
        int xMax = std::max(clicks[0].first,  clicks[1].first);
        int yMin = std::min(clicks[0].second, clicks[1].second);
        int yMax = std::max(clicks[0].second, clicks[1].second);
        f.puntos.push_back({ (double)xMin, (double)yMin });   // arriba-izquierda
        f.puntos.push_back({ (double)xMax, (double)yMin });   // arriba-derecha
        f.puntos.push_back({ (double)xMax, (double)yMax });   // abajo-derecha
        f.puntos.push_back({ (double)xMin, (double)yMax });   // abajo-izquierda
        break;
    }

    case FIGURA_CIRCULO:
    {
        // el 1er click es el centro y el 2do es un punto del borde:
        // el radio es la distancia entre los dos (pitagoras)
        double dx = clicks[1].first  - clicks[0].first;
        double dy = clicks[1].second - clicks[0].second;
        f.puntos.push_back({ (double)clicks[0].first, (double)clicks[0].second });
        f.radio = std::sqrt(dx * dx + dy * dy);
        break;
    }
    }
    return f;
}

// --------------------------------------------------------------------
// CALCULAR LAS CASILLAS DE UNA FIGURA
// --------------------------------------------------------------------

// esto sirve para sacar el contorno: llama al algoritmo que le toca a cada tipo
static std::vector<Celda> contornoDeFigura(const Figura& f)
{
    std::vector<Celda> celdas;
    std::vector<Celda> pts = casillasDeLosPuntos(f);

    switch (f.tipo)
    {
    case FIGURA_LINEA:
        dibujarLinea(pts[0].first, pts[0].second, pts[1].first, pts[1].second, celdas);
        break;
    case FIGURA_CIRCULO:
        dibujarCirculo(pts[0].first, pts[0].second, (int)std::lround(f.radio), celdas);
        break;
    default:
        // rectangulo, triangulo y libre: poligono cerrado
        dibujarPoligono(pts, true, celdas);
        break;
    }
    return celdas;
}

// esto sirve para sacar el interior (la linea no tiene interior)
static bool puntoEnPoligono(const std::vector<Punto>& puntos, double x, double y)
{
    if (puntos.size() < 3) return false;
    bool dentro = false;
    for (size_t i = 0, j = puntos.size() - 1; i < puntos.size(); j = i++)
    {
        const Punto& a = puntos[i];
        const Punto& b = puntos[j];
        double cruz = (x - a.x) * (b.y - a.y) - (y - a.y) * (b.x - a.x);
        double minX = std::min(a.x, b.x) - 1e-8, maxX = std::max(a.x, b.x) + 1e-8;
        double minY = std::min(a.y, b.y) - 1e-8, maxY = std::max(a.y, b.y) + 1e-8;
        if (std::fabs(cruz) < 1e-8 && x >= minX && x <= maxX && y >= minY && y <= maxY)
            return true; // tambien cuenta el borde
        if (((a.y > y) != (b.y > y)) &&
            (x < (b.x - a.x) * (y - a.y) / (b.y - a.y) + a.x))
            dentro = !dentro;
    }
    return dentro;
}

static bool celdaPerteneceFigura(const Figura& f, int columna, int fila)
{
    if (f.tipo == FIGURA_LINEA || f.puntos.empty()) return false;
    if (f.tipo == FIGURA_CIRCULO)
    {
        double dx = (double)columna - f.puntos[0].x;
        double dy = (double)fila - f.puntos[0].y;
        double radio = f.radio + 0.5;
        return dx * dx + dy * dy <= radio * radio;
    }
    // Las coordenadas representan casillas; se prueba el centro de cada casilla.
    return puntoEnPoligono(f.puntos, columna + 0.5, fila + 0.5);
}

// Interior calculado directamente con la geometria, sin depender de que
// el contorno rasterizado cierre perfectamente todos sus pixeles.
static std::vector<Celda> interiorDeFigura(const Figura& f)
{
    std::vector<Celda> interior;
    if (f.tipo == FIGURA_LINEA || f.puntos.empty()) return interior;

    double minX, maxX, minY, maxY;
    if (f.tipo == FIGURA_CIRCULO)
    {
        minX = f.puntos[0].x - f.radio - 1.0;
        maxX = f.puntos[0].x + f.radio + 1.0;
        minY = f.puntos[0].y - f.radio - 1.0;
        maxY = f.puntos[0].y + f.radio + 1.0;
    }
    else
    {
        minX = maxX = f.puntos[0].x;
        minY = maxY = f.puntos[0].y;
        for (size_t i = 1; i < f.puntos.size(); ++i)
        {
            minX = std::min(minX, f.puntos[i].x); maxX = std::max(maxX, f.puntos[i].x);
            minY = std::min(minY, f.puntos[i].y); maxY = std::max(maxY, f.puntos[i].y);
        }
    }

    int x0 = std::max(0, (int)std::floor(minX));
    int x1 = std::min(columnas - 1, (int)std::ceil(maxX));
    int y0 = std::max(0, (int)std::floor(minY));
    int y1 = std::min(filas - 1, (int)std::ceil(maxY));

    const std::vector<Celda> borde = contornoDeFigura(f);
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x)
            if (celdaPerteneceFigura(f, x, y) &&
                std::find(borde.begin(), borde.end(), Celda(x, y)) == borde.end())
                interior.push_back({ x, y });
    return interior;
}

// --------------------------------------------------------------------
// REPINTAR LA GRILLA
// esto sirve para dibujar todo: limpio la grilla y pinto las figuras en
// orden (la ultima queda encima). si la figura esta rellena, pinto primero
// el interior y luego el contorno encima.
// --------------------------------------------------------------------
void repintarGrilla()
{
    limpiarGrilla();

    for (size_t i = 0; i < figuras.size(); i++)
    {
        if (figuras[i].relleno)
            pintarCeldas(interiorDeFigura(figuras[i]), figuras[i].colorRelleno);

        pintarCeldas(contornoDeFigura(figuras[i]), figuras[i].color);
    }

    // los clicks de la figura que todavia no termino: pinto cada punto y
    // las lineas que ya quedaron entre ellos (sin seguir al mouse)
    if (enCurso.activa)
    {
        std::vector<Celda> celdas;
        dibujarPoligono(enCurso.clicks, false, celdas);
        pintarCeldas(celdas, enCurso.color);
    }
}

// --------------------------------------------------------------------
// AGREGAR UNA FIGURA (y mostrarla por consola)
// --------------------------------------------------------------------

// esto sirve para escribir una descripcion corta de la figura
static void describirFigura(const Figura& f, char* texto, size_t tamano)
{
    std::vector<Celda> p = casillasDeLosPuntos(f);

    switch (f.tipo)
    {
    case FIGURA_LINEA:
        snprintf(texto, tamano, "Linea: (%d, %d) a (%d, %d)", p[0].first, p[0].second, p[1].first, p[1].second);
        break;
    case FIGURA_RECTANGULO:
    {
        int ancho = p[1].first - p[0].first + 1;
        int alto  = p[3].second - p[0].second + 1;
        if (ancho == alto)
            snprintf(texto, tamano, "Cuadrado: lado %d", ancho);
        else
            snprintf(texto, tamano, "Rectangulo: %d x %d casillas", ancho, alto);
        break;
    }
    case FIGURA_CIRCULO:
        snprintf(texto, tamano, "Circulo: centro (%d, %d), radio %d", p[0].first, p[0].second, (int)std::lround(f.radio));
        break;
    case FIGURA_TRIANGULO:
        snprintf(texto, tamano, "Triangulo: (%d, %d) (%d, %d) (%d, %d)",
                 p[0].first, p[0].second, p[1].first, p[1].second, p[2].first, p[2].second);
        break;
    default:
        snprintf(texto, tamano, "Forma libre: %d vertices", (int)p.size());
        break;
    }
}

// esto sirve para ver por consola, paso a paso, las casillas que calculo el
// algoritmo (me sirve para explicarlo en el video)
void agregarFigura(const Figura& figura)
{
    guardarParaDeshacer();
    figuras.push_back(figura);
    grillaCambio = true;

    char titulo[128];
    describirFigura(figura, titulo, sizeof(titulo));

    std::vector<Celda> celdas = contornoDeFigura(figura);
    std::cout << titulo << " (" << celdas.size() << " casillas):" << std::endl;
    for (size_t k = 0; k < celdas.size(); k++)
        std::cout << "  paso " << k << " -> (col " << celdas[k].first
                  << ", fila " << celdas[k].second << ")" << std::endl;
}

void cancelarFiguraEnCurso()
{
    if (!enCurso.activa) return;
    enCurso.activa = false;
    enCurso.clicks.clear();
    grillaCambio = true;
}

// --------------------------------------------------------------------
// ELEGIR UNA FIGURA CON EL MOUSE
// esto sirve para saber sobre cual figura hice click. reviso desde la
// ultima (la que esta encima) hasta la primera, y miro tanto el contorno
// como el interior (asi puedo hacer click adentro de una figura sin rellenar).
// --------------------------------------------------------------------
int figuraEnCelda(int columna, int fila)
{
    Celda buscada(columna, fila);
    for (int i = (int)figuras.size() - 1; i >= 0; --i)
    {
        const Figura& f = figuras[i];
        std::vector<Celda> contorno = contornoDeFigura(f);
        if (std::find(contorno.begin(), contorno.end(), buscada) != contorno.end() ||
            celdaPerteneceFigura(f, columna, fila))
            return i;
    }
    return -1;
}

void alternarRelleno(int indice)
{
    if (indice < 0 || indice >= (int)figuras.size()) return;
    guardarParaDeshacer();
    figuras[indice].relleno = !figuras[indice].relleno;
    grillaCambio = true;
}

// Aplica el color elegido al contorno y activa el relleno de la figura.
void aplicarColorFigura(int indice, Color color, Color colorRelleno)
{
    if (indice < 0 || indice >= (int)figuras.size()) return;
    guardarParaDeshacer();
    figuras[indice].color = color;
    figuras[indice].colorRelleno = colorRelleno;
    figuras[indice].relleno = true;
    grillaCambio = true;
}

// Rellena la figura con el color elegido sin modificar el color del borde.
void aplicarRellenoFigura(int indice, Color colorRelleno)
{
    if (indice < 0 || indice >= (int)figuras.size()) return;
    if (figuras[indice].tipo == FIGURA_LINEA) return;
    guardarParaDeshacer();
    figuras[indice].colorRelleno = colorRelleno;
    figuras[indice].relleno = true;
    grillaCambio = true;
}

// --------------------------------------------------------------------
// TRANSFORMACIONES LINEALES
// una transformacion lineal en 2D es multiplicar cada punto por una
// matriz 2x2:
//
//      | x' |   | a  b |   | x |
//      |    | = |      | * |   |        es decir:  x' = a*x + b*y
//      | y' |   | c  d |   | y |                   y' = c*x + d*y
//
// OJO: eso gira o escala alrededor del origen (0, 0), que es la esquina
// de la grilla. yo quiero que la figura gire sobre SU PROPIO centro, entonces:
//   1) resto el centro a cada punto (muevo la figura al origen)
//   2) multiplico por la matriz
//   3) vuelvo a sumar el centro
// --------------------------------------------------------------------

// esto sirve para encontrar el centro de la figura (el promedio de sus puntos)
static Punto centroDeFigura(const Figura& f)
{
    Punto centro = { 0.0, 0.0 };
    for (size_t i = 0; i < f.puntos.size(); i++)
    {
        centro.x += f.puntos[i].x;
        centro.y += f.puntos[i].y;
    }
    centro.x /= f.puntos.size();
    centro.y /= f.puntos.size();
    return centro;
}

// esto sirve para aplicar cualquier matriz 2x2 a una figura, alrededor de su centro
static void aplicarMatriz(Figura& f, double a, double b, double c, double d)
{
    Punto centro = centroDeFigura(f);

    for (size_t i = 0; i < f.puntos.size(); i++)
    {
        double dx = f.puntos[i].x - centro.x;   // 1) llevo el punto al origen
        double dy = f.puntos[i].y - centro.y;

        f.puntos[i].x = centro.x + a * dx + b * dy;   // 2) multiplico y 3) regreso
        f.puntos[i].y = centro.y + c * dx + d * dy;
    }

    // el radio del circulo cambia por la raiz del determinante de la matriz
    // (con una rotacion el determinante es 1, asi que el radio no cambia)
    if (f.tipo == FIGURA_CIRCULO)
        f.radio = f.radio * std::sqrt(std::fabs(a * d - b * c));
}

// esto sirve para rotar: la matriz de rotacion es  | cos  -sin |
//                                                  | sin   cos |
// como la Y de la pantalla crece hacia abajo, un angulo positivo gira
// en el sentido de las manecillas del reloj
void rotarFigura(int indice, double grados)
{
    if (indice < 0 || indice >= (int)figuras.size()) return;

    guardarParaDeshacer();
    double radianes = grados * PI / 180.0;
    aplicarMatriz(figuras[indice], std::cos(radianes), -std::sin(radianes),
                                   std::sin(radianes),  std::cos(radianes));
    grillaCambio = true;
}

// esto sirve para escalar: la matriz de escala es  | s  0 |
//                                                  | 0  s |
void escalarFigura(int indice, double factor)
{
    if (indice < 0 || indice >= (int)figuras.size()) return;

    guardarParaDeshacer();
    aplicarMatriz(figuras[indice], factor, 0.0, 0.0, factor);
    grillaCambio = true;
}
