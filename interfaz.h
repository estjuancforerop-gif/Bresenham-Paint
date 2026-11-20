// ====================================================================
//  interfaz.h
//  Autor: Juan Camilo Forero Paez
//
//  esto sirve para: todo lo que el usuario ve y toca. la barra de
//  herramientas (hecha con Dear ImGui) y la lectura del mouse y teclado.
// ====================================================================
#pragma once

struct GLFWwindow;   // solo aviso que existe, no necesito todo GLFW aqui

// tamanos de la ventana (en pixeles)
const int ALTO_BARRA_PX    = 70;    // alto de la barra de herramientas
const int ANCHO_INICIAL_PX = 800;   // ancho de la ventana al abrir
const int ALTO_INICIAL_PX  = 870;   // alto al abrir (barra + 800 de grilla)
const int ANCHO_MINIMO_PX  = 400;   // ancho minimo para la barra
const int ALTO_MINIMO_PX   = 300;

// esto sirve para saber en que parte de la ventana esta la grilla
// (esta justo debajo de la barra y ocupa todo lo que sobra)
struct Lienzo
{
    float x, y;          // esquina de arriba-izquierda
    float ancho, alto;   // tamano
};
extern Lienzo lienzo;

void calcularLienzo();          // recalcula el lienzo con el tamano actual de la ventana

void iniciarGUI(GLFWwindow* window);
void comenzarCuadroGUI();
void terminarCuadroGUI();
void cerrarGUI();

void dibujarBarra();             // la barra de herramientas
void procesarMouse();            // clicks sobre la grilla
void procesarTeclado();          // atajos (Ctrl+Z, Supr, Esc, Enter)
void dibujarCasillaBajoMouse();  // borde blanco en la casilla donde esta el mouse
