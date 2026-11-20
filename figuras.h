// ====================================================================
//  figuras.h
//  Autor: Juan Camilo Forero Paez
//
//  esto sirve para: guardar la lista de figuras que dibujo y todo lo que
//  les puedo hacer (rellenar, rotar, escalar, deshacer).
//  la grilla no guarda las figuras, solo los colores. las figuras viven
//  aqui y la grilla se repinta desde cero cada vez que algo cambia.
// ====================================================================
#pragma once
#include <vector>
#include "grilla.h"

// esto sirve para guardar un punto con decimales. uso decimales (y no
// casillas enteras) para que al rotar varias veces la figura no se vaya
// deformando por los redondeos
struct Punto
{
    double x, y;
};

enum TipoFigura
{
    FIGURA_LINEA,
    FIGURA_RECTANGULO,   // un cuadrado es un rectangulo con los lados iguales
    FIGURA_CIRCULO,
    FIGURA_TRIANGULO,
    FIGURA_LIBRE         // poligono con los puntos que yo quiera
};

// esto sirve para guardar una figura ya terminada:
//   linea      -> 2 puntos
//   rectangulo -> 4 puntos (las 4 esquinas)
//   triangulo  -> 3 puntos
//   libre      -> n puntos
//   circulo    -> puntos[0] es el centro y ademas guarda el radio
struct Figura
{
    TipoFigura         tipo;
    std::vector<Punto> puntos;
    double             radio;    // solo lo usa el circulo
    bool               relleno;  // true = la figura esta rellena
    Color              color;
    Color              colorRelleno; // color independiente del interior
};

// esto sirve para recordar los clicks de la figura que todavia no termino
// (por ejemplo, el triangulo cuando llevo 2 de sus 3 clicks)
struct FiguraEnCurso
{
    bool               activa;
    TipoFigura         tipo;
    Color              color;
    std::vector<Celda> clicks;
};

extern std::vector<Figura> figuras;   // todas las figuras de la grilla (en orden)
extern FiguraEnCurso       enCurso;   // la figura que estoy haciendo
extern bool                grillaCambio;  // avisa que hay que repintar la grilla

// ---- crear y borrar ----
Figura crearFigura(TipoFigura tipo, const std::vector<Celda>& clicks, Color color);
void   agregarFigura(const Figura& figura);
void   cancelarFiguraEnCurso();
void   deshacer();
void   borrarTodo();

// ---- pintar ----
void repintarGrilla();

// ---- elegir una figura con el mouse ----
int figuraEnCelda(int columna, int fila);   // indice de la figura (o -1 si no hay)

// ---- lo que le puedo hacer a una figura ----
void alternarRelleno(int indice);
void aplicarColorFigura(int indice, Color color, Color colorRelleno);
void aplicarRellenoFigura(int indice, Color colorRelleno);
void rotarFigura(int indice, double grados);
void escalarFigura(int indice, double factor);
