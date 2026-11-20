// ====================================================================
//  interfaz.cpp
//  Autor: Juan Camilo Forero Paez
// ====================================================================
#include "interfaz.h"
#include "figuras.h"
#include "grilla.h"

#include <algorithm>
#include <cmath>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

Lienzo lienzo = { 0.0f, (float)ALTO_BARRA_PX, (float)ANCHO_INICIAL_PX,
                  (float)(ALTO_INICIAL_PX - ALTO_BARRA_PX) };

// --------------------------------------------------------------------
// LAS HERRAMIENTAS
// las primeras 4 dibujan figuras. las ultimas 2 se aplican a una figura
// que ya existe (hago click sobre ella).
// --------------------------------------------------------------------
enum Herramienta
{
    HERRAMIENTA_LINEA,
    HERRAMIENTA_CUADRADO,    // con dos clicks tambien sale rectangulo
    HERRAMIENTA_CIRCULO,
    HERRAMIENTA_TRIANGULO,
    HERRAMIENTA_RELLENO,
    HERRAMIENTA_ROTAR
};

static Herramienta herramientaActual = HERRAMIENTA_LINEA;
static Color colorSeleccionado = { 0.95f, 0.55f, 0.15f };
static Color colorRellenoSeleccionado = { 0.45f, 0.20f, 0.05f };

// cuanto gira cada click con la herramienta de rotar (en grados)
static const double GRADOS_POR_CLICK = 15.0;
// cuanto crece cada click con la herramienta de escalar
static const double FACTOR_POR_CLICK = 1.25;

// la casilla donde esta el mouse (se calcula en cada cuadro)
static int  mouseColumna = 0, mouseFila = 0;
static bool mouseDentroGrilla = false;

// --------------------------------------------------------------------
// datos de cada herramienta
// --------------------------------------------------------------------

// esto sirve para saber que tipo de figura crea cada herramienta de dibujo
static TipoFigura tipoDeHerramienta(Herramienta h)
{
    switch (h)
    {
    case HERRAMIENTA_LINEA:     return FIGURA_LINEA;
    case HERRAMIENTA_CIRCULO:   return FIGURA_CIRCULO;
    case HERRAMIENTA_TRIANGULO: return FIGURA_TRIANGULO;
    default:                    return FIGURA_RECTANGULO;
    }
}

// esto sirve para saber cuantos clicks necesita cada herramienta de dibujo.
static int clicksNecesarios(Herramienta h)
{
    switch (h)
    {
    case HERRAMIENTA_TRIANGULO: return 3;
    default:                    return 2;
    }
}

static bool esHerramientaDeDibujo(Herramienta h)
{
    return h <= HERRAMIENTA_TRIANGULO;
}

// esto sirve para dejar un numero dentro de un rango (ej: la columna entre 0 y columnas-1)
static int limitar(int valor, int minimo, int maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}

// --------------------------------------------------------------------
// EL LIENZO Y EL MOUSE
// --------------------------------------------------------------------

// esto sirve para saber donde queda la grilla en la ventana. la grilla
// empieza justo debajo de la barra y ocupa todo el espacio que sobra
void calcularLienzo()
{
    ImVec2 ventana = ImGui::GetIO().DisplaySize;

    lienzo.x = 0.0f;
    lienzo.y = (float)ALTO_BARRA_PX;
    lienzo.ancho = std::max(ventana.x, 1.0f);
    lienzo.alto  = std::max(ventana.y - (float)ALTO_BARRA_PX, 1.0f);
}

// esto sirve para convertir los pixeles del mouse en (columna, fila).
// resto la esquina del lienzo y divido entre el tamano de una casilla
static void actualizarCasillaDelMouse()
{
    ImVec2 m = ImGui::GetMousePos();

    float x = m.x - lienzo.x;
    float y = m.y - lienzo.y;

    // la grilla solo llega hasta columnas*20 pixeles (puede ser menos que el lienzo)
    mouseDentroGrilla = (x >= 0 && x < columnas * TAMANO_CASILLA_PX &&
                         y >= 0 && y < filas * TAMANO_CASILLA_PX);

    mouseColumna = limitar((int)std::floor(x / TAMANO_CASILLA_PX), 0, columnas - 1);
    mouseFila    = limitar((int)std::floor(y / TAMANO_CASILLA_PX), 0, filas - 1);
}

// esto sirve para ver exactamente en que casilla voy a hacer click
void dibujarCasillaBajoMouse()
{
    if (!mouseDentroGrilla || ImGui::GetIO().WantCaptureMouse) return;

    ImVec2 a(lienzo.x + mouseColumna * TAMANO_CASILLA_PX, lienzo.y + mouseFila * TAMANO_CASILLA_PX);
    ImVec2 b(a.x + TAMANO_CASILLA_PX, a.y + TAMANO_CASILLA_PX);

    // el foreground dibuja encima de todo
    ImGui::GetForegroundDrawList()->AddRect(a, b, IM_COL32(255, 255, 255, 255), 0.0f, 0, 2.0f);
}

// --------------------------------------------------------------------
// LO QUE PASA CON CADA CLICK
// --------------------------------------------------------------------

// esto sirve para que con Shift el segundo click del cuadrado salga con los
// dos lados iguales (cuadrado perfecto) sin salirse de la grilla
static Celda ajustarACuadrado(Celda inicio, Celda fin)
{
    int dx = fin.first  - inicio.first;
    int dy = fin.second - inicio.second;

    int signoX = (dx < 0) ? -1 : 1;
    int signoY = (dy < 0) ? -1 : 1;

    // cuanto espacio queda hacia donde voy, hasta el borde de la grilla
    int libreX = (signoX < 0) ? inicio.first  : (columnas - 1 - inicio.first);
    int libreY = (signoY < 0) ? inicio.second : (filas - 1 - inicio.second);

    int lado = std::max(std::abs(dx), std::abs(dy));
    lado = std::min(lado, std::min(libreX, libreY));

    return Celda(inicio.first + signoX * lado, inicio.second + signoY * lado);
}

// esto sirve para terminar la figura: la creo con los clicks que llevo y la agrego a la lista
static void terminarFiguraEnCurso()
{
    Figura f = crearFigura(enCurso.tipo, enCurso.clicks, enCurso.color);
    enCurso.activa = false;
    enCurso.clicks.clear();
    agregarFigura(f);
}

// esto sirve para las herramientas de dibujo: cada click agrega un punto
static void clickDeDibujo()
{
    ImGuiIO& io = ImGui::GetIO();
    Celda click(mouseColumna, mouseFila);

    if (!enCurso.activa)
    {
        // primer click: empiezo una figura nueva
        enCurso.activa = true;
        enCurso.tipo   = tipoDeHerramienta(herramientaActual);
        enCurso.color  = colorSeleccionado;
        enCurso.clicks.clear();
    }
    else if (herramientaActual == HERRAMIENTA_CUADRADO && io.KeyShift)
    {
        click = ajustarACuadrado(enCurso.clicks[0], click);
    }

    enCurso.clicks.push_back(click);
    grillaCambio = true;   // para que se marque el punto

    // si ya tengo todos los clicks, termino la figura (la libre no termina sola)
    int necesarios = clicksNecesarios(herramientaActual);
    if (necesarios > 0 && (int)enCurso.clicks.size() >= necesarios)
        terminarFiguraEnCurso();
}

// esto sirve para las 3 herramientas que se aplican a una figura que ya existe
static void clickSobreFigura()
{
    int indice = figuraEnCelda(mouseColumna, mouseFila);
    if (indice < 0) return;   // hice click en un lugar vacio

    bool alReves = ImGui::GetIO().KeyShift;   // con Shift hace lo contrario

    switch (herramientaActual)
    {
    case HERRAMIENTA_RELLENO:
        aplicarRellenoFigura(indice, colorRellenoSeleccionado);
        break;
    case HERRAMIENTA_ROTAR:
        rotarFigura(indice, alReves ? -GRADOS_POR_CLICK : GRADOS_POR_CLICK);
        break;
    default:
        break;
    }
}

// esto sirve para revisar el mouse en cada cuadro
void procesarMouse()
{
    ImGuiIO& io = ImGui::GetIO();
    actualizarCasillaDelMouse();

    // io.WantCaptureMouse es true cuando el mouse esta sobre la barra:
    // ahi NO se dibuja, ahi solo se elige la herramienta
    bool sobreGrilla = mouseDentroGrilla && !io.WantCaptureMouse;

    if (!sobreGrilla || !ImGui::IsMouseClicked(ImGuiMouseButton_Left)) return;

    if (esHerramientaDeDibujo(herramientaActual))
        clickDeDibujo();
    else
        clickSobreFigura();
}

// esto sirve para los atajos del teclado:
//   Esc = cancelar la figura que estoy haciendo, Ctrl+Z = deshacer, Supr = borrar todo
void procesarTeclado()
{
    ImGuiIO& io = ImGui::GetIO();

    if (enCurso.activa)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
            cancelarFiguraEnCurso();
        return;   // mientras hago una figura no deshago ni borro nada
    }

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z))
        deshacer();
    if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
        borrarTodo();
}

// ====================================================================
//  LA BARRA DE HERRAMIENTAS (Dear ImGui)
//
//  ImGui funciona en "modo inmediato": en CADA cuadro vuelvo a describir
//  la barra y ImGui me dice al instante si tocaron algo. por eso no
//  guardo botones como objetos: solo escribo
//      if (ImGui::Button(...)) { ... }
// ====================================================================

// esto sirve para conectar ImGui con GLFW (eventos) y OpenGL (dibujo)
void iniciarGUI(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsLight();   // colores claros, tipo Paint

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void comenzarCuadroGUI()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void terminarCuadroGUI()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void cerrarGUI()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

// --------------------------------------------------------------------
// ICONOS
// esto sirve para dibujar el icono de cada boton con lineas y formas,
// sin usar imagenes. cada funcion recibe la esquina de arriba-izquierda (a)
// y la de abajo-derecha (b) del espacio donde se dibuja.
// --------------------------------------------------------------------
typedef void (*FuncionIcono)(ImDrawList*, ImVec2, ImVec2, ImU32);

static void iconoLinea(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 color)
{
    dl->AddLine(ImVec2(a.x, b.y), ImVec2(b.x, a.y), color, 3.0f);
}

static void iconoCuadrado(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 color)
{
    dl->AddRect(a, b, color, 0.0f, 0, 3.0f);
}

static void iconoCirculo(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 color)
{
    ImVec2 centro((a.x + b.x) / 2, (a.y + b.y) / 2);
    dl->AddCircle(centro, (b.x - a.x) / 2, color, 0, 3.0f);
}

static void iconoTriangulo(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 color)
{
    dl->AddTriangle(ImVec2((a.x + b.x) / 2, a.y), ImVec2(b.x, b.y), ImVec2(a.x, b.y), color, 3.0f);
}

static void iconoRelleno(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 color)
{
    dl->AddRectFilled(a, b, ImGui::ColorConvertFloat4ToU32(ImVec4(colorSeleccionado.r, colorSeleccionado.g, colorSeleccionado.b, 1.0f)));
    dl->AddRect(a, b, color, 0.0f, 0, 3.0f);
}

static void iconoRotar(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 color)
{
    // un arco de 270 grados con una flecha en la punta
    ImVec2 centro((a.x + b.x) / 2, (a.y + b.y) / 2);
    float radio = (b.x - a.x) / 2;
    float fin = 4.7f;   // angulo final del arco (en radianes)

    dl->PathArcTo(centro, radio, 0.0f, fin, 24);
    dl->PathStroke(color, 0, 3.0f);

    // punta de la flecha: sale del final del arco, en la direccion en la que giro
    ImVec2 punta(centro.x + radio * std::cos(fin), centro.y + radio * std::sin(fin));
    ImVec2 tangente(-std::sin(fin), std::cos(fin));
    ImVec2 normal(std::cos(fin), std::sin(fin));
    dl->AddTriangleFilled(
        ImVec2(punta.x + tangente.x * 7, punta.y + tangente.y * 7),
        ImVec2(punta.x + normal.x * 5, punta.y + normal.y * 5),
        ImVec2(punta.x - normal.x * 5, punta.y - normal.y * 5), color);
}

// --------------------------------------------------------------------
// UN BOTON
// esto sirve para dibujar un boton cuadrado con su icono (sin texto).
// el que esta elegido se ve azul.
// --------------------------------------------------------------------
static void botonHerramienta(Herramienta herramienta, FuncionIcono icono)
{
    bool elegida = (herramientaActual == herramienta);

    ImGui::PushID((int)herramienta);   // cada boton necesita un id distinto

    if (elegida)
    {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.55f, 0.75f, 0.98f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.75f, 0.98f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border,        ImVec4(0.10f, 0.40f, 0.80f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.90f, 0.90f, 0.90f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.82f, 0.90f, 0.99f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border,        ImVec4(0.70f, 0.70f, 0.70f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    }

    // Si vuelvo a pulsar la herramienta de figura que ya estaba activa,
    // salgo del modo de dibujo y paso directamente al modo de relleno.
    // Asi el siguiente clic dentro de una figura la pinta, no crea otra.
    if (ImGui::Button("##boton", ImVec2(54, 54)))
    {
        if (herramientaActual == herramienta && esHerramientaDeDibujo(herramienta))
            herramientaActual = HERRAMIENTA_RELLENO;
        else
            herramientaActual = herramienta;

        cancelarFiguraEnCurso();   // descarto cualquier figura que estuviera a medias
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    ImGui::PopID();

    // el icono se dibuja ENCIMA del boton, dejando 14 pixeles de margen
    ImVec2 esqA = ImGui::GetItemRectMin();
    ImVec2 esqB = ImGui::GetItemRectMax();
    icono(ImGui::GetWindowDrawList(),
          ImVec2(esqA.x + 14, esqA.y + 14), ImVec2(esqB.x - 14, esqB.y - 14),
          IM_COL32(40, 40, 40, 255));
}

// --------------------------------------------------------------------
// LA BARRA COMPLETA
// esto sirve para tener una barra pegada arriba, de todo el ancho, con
// los 8 botones en fila
// --------------------------------------------------------------------
void dibujarBarra()
{
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, (float)ALTO_BARRA_PX));

    ImGuiWindowFlags banderas = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                                ImGuiWindowFlags_NoScrollbar;
    ImGui::Begin("Barra", nullptr, banderas);

    // botones principales de dibujo y edicion
    struct Boton { Herramienta herramienta; FuncionIcono icono; };
    const Boton botones[] = {
        { HERRAMIENTA_LINEA,     iconoLinea     },
        { HERRAMIENTA_CUADRADO,  iconoCuadrado  },
        { HERRAMIENTA_CIRCULO,   iconoCirculo   },
        { HERRAMIENTA_TRIANGULO, iconoTriangulo },
        { HERRAMIENTA_RELLENO,   iconoRelleno   },
        { HERRAMIENTA_ROTAR,     iconoRotar     }
    };

    for (int i = 0; i < 6; i++)
    {
        if (i > 0) ImGui::SameLine();
        botonHerramienta(botones[i].herramienta, botones[i].icono);
        if (botones[i].herramienta == HERRAMIENTA_RELLENO && ImGui::IsItemClicked())
            ImGui::OpenPopup("Catalogo de colores");
    }

    ImGui::SameLine();
    if (herramientaActual == HERRAMIENTA_RELLENO)
        ImGui::TextColored(ImVec4(0.15f, 0.65f, 0.25f, 1.0f), "Modo: RELLENAR (clic dentro)");
    else if (esHerramientaDeDibujo(herramientaActual))
        ImGui::TextUnformatted("Modo: dibujar (pulsa otra vez para rellenar)");
    else
        ImGui::TextUnformatted("Modo: editar figura");

    // Selector de color: permite elegir un tono personalizado (RGB/HSV).
    if (ImGui::BeginPopup("Catalogo de colores"))
    {
        ImGui::TextUnformatted("Color del contorno:");
        ImGui::ColorPicker3("##colorfigura", &colorSeleccionado.r,
            ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHSV |
            ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview);
        ImGui::Separator();
        ImGui::TextUnformatted("Color para rellenar:");
        ImGui::ColorPicker3("##colorrelleno", &colorRellenoSeleccionado.r,
            ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHSV |
            ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview);
        ImGui::Separator();
        ImGui::TextWrapped("Elige el color interior y pulsa Aplicar. Despues haz clic dentro de una figura.");
        if (ImGui::Button("Aplicar colores", ImVec2(-1, 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::End();
}
