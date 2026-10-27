#v1.0.4
#Implementado un sistema para que sea posible añadir resources al build
#v1.0.3
#Implementado para que el color no de error en linux
#Mejoras en la calidad del codigo
#v1.0.2
#Implementado el flag -e para ejecutar automaticamente el ejecutable compilado
#Implementada la forma para pasar argumentos al ejecutar el ejecutable compilados
#v1.0.0.1
#ahora se escribe un archivo compile_commands.json

includeDirs = ["-Iexternal/includes"] #Lista de dirs a incluir, cada dir tem de ser precedido por -I
libsDirs = ["-Lexternal/libs"] #Lista de dirs de libs, cada dir tem de ser precedido por -L
libs = {
    "debug": [],
    "release": ["-static-libgcc", "-static-libstdc++"],
    "all": ["-lavcodec","-lavutil","-lswscale","-limGUI_SDL3_Vulkan","-lSDL3","-lvulkan-1","-lstb_image","-ld3d11","-ldxgi","-lws2_32","-liphlpapi","-lwinhttp","-lpthread","-lsodium","-lmfplat","-lmfuuid","-lole32"]
}


from pathlib import Path
from datetime import datetime
import subprocess
import os
import argparse
import ctypes
import platform
import subprocess
import ctypes
import sys
import re

#Configurações de paths, extensões, libs e flags
compilationPath = Path("compilationFiles") #Path onde fica todo o relacionado com a compilação
sourcePath = Path("src") #Path onde estão os ficheiros cpp
filesExtension = "cpp" #Extensão dos ficheiros do projeto
compilatedFilesExtension = "o" #Extensão dos ficheiros compilados
buildPath = Path("out") #Path onde ficam os executáveis finais
compilator = "g++" #Compilador a usar, deve estar no PATH do sistema
generatedPath = Path("build/generated")
resourcesPath = Path("resources")

if sys.platform == "win32":
    import ctypes
    kernel32 = ctypes.windll.kernel32
    handle = kernel32.GetStdHandle(-11)  # STD_OUTPUT_HANDLE
    mode = ctypes.c_ulong()
    kernel32.GetConsoleMode(handle, ctypes.byref(mode))
    kernel32.SetConsoleMode(
        handle,
        mode.value | 0x0004  # ENABLE_VIRTUAL_TERMINAL_PROCESSING
    )
    os.system("color")
# en Linux/macOS no se necesita nada más: los ANSI escapes funcionan por defecto


def verifyArguments(_compilationMode: str, _specificFlags: dict):
    if _compilationMode not in _specificFlags:
        print(f"Invalid compilation mode: {_compilationMode}. Use or create flags for compilation mode.")
        for _mode in _specificFlags.keys():
         print(f" - {_mode}")
        exit(1)
    elif _compilationMode == "all":
        print("Compilation mode 'all' is not valid for compilation.")
        exit(1)

parser = argparse.ArgumentParser()
parser.add_argument("compilation_mode")
parser.add_argument("-e", "--execute", action="store_true", help="Ejecutar tras compilar")
parser.add_argument("program_args", nargs="*", help="Argumentos para el programa a ejecutar")
args = parser.parse_args()

compilationMode = args.compilation_mode.lower()
flagExecute = args.execute
programArgs = args.program_args  # lista de strings


#Configurações do projeto
projectName = Path.cwd().name
projectVersion = "0.0.0"
engineVersion = "0.0.0"
flags = {
    "debug": [
        "-g",
        "-O0",
        "-D_DEBUG",
        # Evita que el ejecutable dependa de las DLL de runtime de MinGW
        # (libstdc++-6.dll y libgcc_s_*-1.dll) al iniciarse.
        "-static-libgcc",
        "-static-libstdc++",
    ],
    "release": [
        "-O3",
    ],
    "all": [
        f"-DPROJECT_NAME=\"{projectName}\"",
        "-MMD", #Gera um ficheiro .d com as dependências de cada ficheiro .cpp
        "-MP", #Gera um ficheiro .d mesmo que o ficheiro .cpp não tenha dependências, evitando erros de "file not found" quando um ficheiro .h é eliminado
        f"-DPROJECT_VERSION=\"{projectVersion}\"",
        f"-DENGINE_VERSION=\"{engineVersion}\"",
        "-DVK_USE_PLATFORM_WIN32_KHR "
        "-fdiagnostics-color=never",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-Wno-unused-result",
    ]
}
verifyArguments(compilationMode, flags)


#Paths onde ficam os ficheiros compilados e os executáveis finais, cada um tem uma subpasta para cada modo de compilação   
compilatedFilesPath = compilationPath / compilationMode
finalBuildPath = buildPath / compilationMode

compileCommand = [
    compilator,
    *flags[compilationMode],
    *flags["all"],
    *includeDirs,
    "-c"
    ]

print("Compile command: ")
print(" ".join(compileCommand))




generatedPath.mkdir(parents=True, exist_ok=True)

lines = ["#pragma once", "#include <cstddef>", ""]
if resourcesPath.exists():
    for fileName in sorted(resourcesPath.rglob("*")):
        if not fileName.is_file():
            continue
        data = fileName.read_bytes()
        if not data:
            continue
        name = re.sub(r"\W", "_", fileName.stem.upper())
        arr = ",".join(map(str, data))
        lines.append(f"inline constexpr unsigned char {name}_DATA[] = {{{arr}}};")
        lines.append(f"inline constexpr std::size_t {name}_SIZE = {len(data)};")
        lines.append("")

newContent = "\n".join(lines)
headerFile = generatedPath / "resources.h"

# Solo reescribir si cambió, para no tocar la fecha de modificación sin necesidad
if not headerFile.exists() or headerFile.read_text() != newContent:
    headerFile.write_text(newContent)

# Procuramos os ficheiros e tempos de modificação do projeto
# Dicionario: Path -> tiempo de modificación
pStems = []
def getFilesDict(_path, extension):
    files_dict = {}
    p = Path(_path)
    if not p.exists():
        print(f"{_path}: doesn't exist.")
        return files_dict
    _f = list(p.rglob(f"*.{extension}"))
    for _file in _f:
        files_dict[_file] = _file.stat().st_mtime
    return files_dict

pFilesTimes = getFilesDict(sourcePath, filesExtension)
numProjectFiles = len(pFilesTimes)

# Ensure output directories exist
for path in [compilatedFilesPath, finalBuildPath]:
    if not Path(path).exists():
        Path(path).mkdir(parents=True, exist_ok=True)


#Retorna um dicionario de todos os headers incluidos por um ficheiro cpp, usando o ficheiro .d gerado na compilação
#As chaves são os Paths dos ficheiros .h e os valores as datas de modificação de cada ficheiro .h
#Caso o ficheiro .d não exista, retorna uma lista de todos os ficheiros .h do projeto, para garantir que o ficheiro .cpp seja compilado
def getIncludedHeaders(_file: Path):
    dFilePath = compilatedFilesPath / f"{_file.stem}.d"
    _headers = {}
    if not dFilePath.exists():
        #Retorna o dicionário de todos os ficheiros.h do projeto
        for _f in Path(sourcePath).rglob("*.h"):
          _headers[_f] = _f.stat().st_mtime
        return _headers
    #Ler o ficheiro.d
    with open(dFilePath, "r") as _f:
        next(_f)
        for line in _f: #A primeira linha do ficheiro .d é o nome do ficheiro .o, por isso começa-se a ler a partir da segunda linha
            if line.strip() == "":
                continue
            headerPath = line.strip().split()[0]
            _header = Path(headerPath)
            if not _header.is_relative_to(sourcePath):
                continue
            if _header.exists():
                _headers[_header] = _header.stat().st_mtime

    return _headers

#As chaves são os Paths dos fiche
#Percorrer a lista de ficheiros do projeto.
#Procura-se o nome desse ficheiro em compilated files.
#Quando se encontrar verifica se a ulima data de compilação é menor do que a ultima 
#acrescenta-se esse ficheiro aos ficheiros para compilar.
#Caso contrario simplesmente continua
#
#Existe outra condição para um ficheiro entrar na lista de ficheiros para compilar:
#Obtem-se a lista de paths de ficheiros  .h aos quais este ficheiro .cpp inclui.
#Se algum desses ficheiros .h tiver uma data de modificação mais recente do que a data de modificação do ficheiro compilado,
#então este ficheiro .cpp deve ser compilado
compilationSuccess = True
import json
compdb = []
for file, time in pFilesTimes.items():
    #Inicializar variaveis
    fileStem = file.stem #Nome do ficheiro cpp
    pStems.append(fileStem) #Adicionar o nome do ficheiro cpp à lista de nomes dos ficheiros do projeto, para depois eliminar os ficheiros compilados que já não existem no projeto
    compilationTime = 0 #Data de modificação do ficheiro compilado
    cppCompilatedPath = compilatedFilesPath / f"{fileStem}.{compilatedFilesExtension}" #Path do ficheiro compilado correspondente a este ficheiro cpp
    compdb.append({
        "directory": str(Path.cwd()),
        "arguments": [*compileCommand, str(file), "-o", str(cppCompilatedPath)],
        "file": str(file),
    })
    #Analize de se é necessario compilar este ficheiro cpp
    if cppCompilatedPath.exists(): #Caso o ficheiro .o exista, obtemos a data de modificação do ficheiro compilado
        compilationTime =cppCompilatedPath.stat().st_mtime

    if compilationTime >= time: #Caso o ficheiro compilado seja mais recente do que o ficheiro cpp e do que a ultima data de build, não é necessário compilar este ficheiro cpp
        #Verificar se algum dos ficheiros.h incluidos pelo ficheiro.cpp foi modificado mais recentemente do que o ficheiro compilado
        headers = getIncludedHeaders(file)
        canCompile = False
        for header, headerTime in headers.items():
            #Se o header foi modificado depois do ficheiro.o ter sido compilado
            if headerTime > compilationTime:
                canCompile = True
                break
        if not canCompile:
            continue

    #Se não foi continue, chega-se à compilação
    commandComplete = compileCommand.copy()
    commandComplete.append(str(file))
    commandComplete.append("-o")
    initialTime = datetime.now() #Medição do tempo
    commandComplete.append(str(cppCompilatedPath))

    #Execução do comando
    result = subprocess.run(commandComplete, capture_output=True, text=True)
        
    #Processamento do resultado da compilação
    if result.stderr:
        print(result.stderr)
    if result.stdout:
        print(result.stdout)
    if result.returncode != 0:
        if sys.platform == "win32":
            ctypes.windll.user32.MessageBeep(0x10) #
            ctypes.windll.user32.MessageBoxW(0, f"Error compiling {file.name}", "Compilation Error", 0)
        print(f"Error compiling {file.name}")
        compilationSuccess = False
    else:
        print(f"Compilated: {file.name} in {(datetime.now()-initialTime).total_seconds()}s")
    print("")

with open("compile_commands.json", "w") as f:
    json.dump(compdb, f, indent=2)

if not compilationSuccess:
    exit(1)

linkingList = []
# Cria a lista para o linking e apaga todos os.o e.d que já não deviam existir
for file in Path(compilatedFilesPath).rglob(f"*.{compilatedFilesExtension}"):
    if file.stem not in pStems:
        os.remove(str(file))
        # Borra el archivo.d asociado si existe
        dep_file = file.with_suffix('.d')
        if dep_file.exists():
            os.remove(str(dep_file))
    else:
        linkingList.append(str(file))



linkCommand = [
    compilator,
    *linkingList,
    *libsDirs,
    *libs[compilationMode],
    *libs["all"],
    *flags[compilationMode],
    *flags["all"],
    "-o",
    str(finalBuildPath / f"{projectName}.exe")
    ]

#Uma vez compilado o linking
initialTime = datetime.now()
result = subprocess.run(linkCommand,capture_output=True,text=True)
if result.stderr:
    print(result.stderr)
if result.stdout:
    print(result.stdout)
if result.returncode != 0:
    if sys.platform == "win32":
        ctypes.windll.user32.MessageBeep(0x10) #
        ctypes.windll.user32.MessageBoxW(0, f"Error linking {projectName}.exe", "Linking Error", 0)
    print(f"Error linking {projectName}.exe")
    exit(1)
else:
    print(f"linked in {(datetime.now()-initialTime).total_seconds()}s")
    if flagExecute:
        exePath = finalBuildPath / f"{projectName}.exe"
        if sys.platform == "win32":
          subprocess.Popen([str(exePath)] + programArgs, creationflags=subprocess.CREATE_NEW_CONSOLE)
        else:  # Linux (o macOS con adaptación)
            #print(["./"+str(exePath)] + programArgs)
            subprocess.run(["./"+str(exePath)] + programArgs)
