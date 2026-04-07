/**
 * @file main.cpp
 * @brief Main file of the project.
 * @note
 * 
 * Al compilar especificas el -D<palavra> o -D<palavra>=<valor> es igual a especificar #define PALAVRA o #define PALAVRA <valor>
 * 
 * Los archivos de inclusion apenas dicen que el codigo existe (archivos.h).
 * Si al compilar usas -I<ruta> el compilador buscara los archivos.h en esa ruta y deja 
 * usar include con <>. Si no usas -I<ruta> el compilador buscara los archivos.h en la carpeta del codigo fuente y deja usar include con "".
 * 
 * Las bibliotecas son como los archivos .c pero ya compilados, es decir, con extension .lib o .dll.
 * Para usar una biblioteca se necesita el archivo .h para incluirlo en el codigo fuente y el archivo .lib o .dll o .a para enlazarlo al compilar.
 * Si al compilar usas -L<ruta> el compilador buscara los archivos .lib o .dll o .a en esa ruta y deja usar -l<nombre> para enlazarlo,
 * eso le dice al compilador para usar ruta/<nombre>.dll.a/.lib/.a.
 * Una libreria puede ser estaticas o dinamicas, las estaticas se enlazan al compilar y las dinamicas se enlazan al ejecutar, es decir, se cargan en memoria al ejecutar el programa.
 * Las dinamicas tienen que usar dll encuanto las estaticas hay que especificar apenas en un define.
 * 
 * El compilador primero hace todo lo del preoprocessador, (#defines y #includes, eliminar comentarios, etc) 
 *  -> Luego procesa el codigo a archivos .o 
 *  -> luego enlaza los archivos .o a las bibliotecas (son equivalentes a los archivos .o) para generar el ejecutable.
 *
 * En el comando de compilar los archivos fuentes van antes de las librerias
 * */


#include <stdio.h>
#include <iostream>
#include <SDL3/SDL.h>


#ifndef PROJECT_NAME /** @brief .exe file and window name*/
#define PROJECT_NAME "Unknown"
#endif
#ifndef PROJECT_VERSION /** @brief Version of the project*/
#define PROJECT_VERSION "0.0.0"
#endif

int main() {
    std::cout << PROJECT_NAME << std::endl;
    std::cout << PROJECT_VERSION << std::endl;
    #ifdef _DEBUG
        std::cout << "Debug mode" << std::endl;
    #else
        std::cout << "Release mode" << std::endl;
    #endif
    return 0;
}