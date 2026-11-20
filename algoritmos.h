// ====================================================================
//  algoritmos.h
//  Autor: Juan Camilo Forero Paez
//
//  esto sirve para: tener juntos los algoritmos de la clase.
//  ninguno dibuja nada: solo calculan QUE casillas hay que pintar y las
//  devuelven en una lista. despues otra funcion las pinta.
// ====================================================================
#pragma once
#include <vector>
#include "grilla.h"

// linea de Bresenham entre dos casillas (sirve en los 8 octantes)
void dibujarLinea(int x0, int y0, int x1, int y1, std::vector<Celda>& celdas);

// circulo con el algoritmo del punto medio (Bresenham para circulos)
void dibujarCirculo(int cx, int cy, int r, std::vector<Celda>& celdas);

// une varios puntos con lineas de Bresenham (triangulos, rectangulos, formas libres)
// si cerrado es true, tambien une el ultimo punto con el primero
void dibujarPoligono(const std::vector<Celda>& esquinas, bool cerrado, std::vector<Celda>& celdas);

// relleno por inundacion: recibe el contorno de una figura y devuelve
// las casillas de ADENTRO (el contorno no va incluido)
std::vector<Celda> rellenarContorno(const std::vector<Celda>& contorno);
