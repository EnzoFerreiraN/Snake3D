# Snake 3D — Trabalho Final de Computação Gráfica

Implementação do clássico jogo **Snake** em três dimensões, desenvolvida em **C++** com **OpenGL/GLUT** como trabalho final da disciplina de Computação Gráfica.

O jogo apresenta uma cobra 3D que se move num grid, come maçãs que aparecem aleatoriamente, cresce a cada refeição e ganha velocidade progressivamente. O mapa é delimitado por uma **borda sólida** de paredes que encerram a partida ao contato. À medida que o jogador acumula maçãs, uma **parede central** — sorteada aleatoriamente como vertical ou horizontal — divide o campo ao meio e **portais** surgem em cada lado dela para teleportar a cobra. Há três modos de câmera (terceira pessoa, primeira pessoa e vista aérea), texturas reais carregadas de arquivos, modelos OBJ e iluminação de Gouraud.

---

## Sumário

- [Controles](#controles)
- [Requisitos](#requisitos)
- [Bibliotecas usadas](#bibliotecas-usadas)
- [Como compilar e executar](#como-compilar-e-executar)
- [Documentação técnica](#documentação-técnica)

---

## Controles

| Tecla / Ação | Efeito |
|---|---|
| **Setas** ou **WASD** | Mudar direção da cobra |
| **C** | Alternar câmera: 3ª pessoa → 1ª pessoa → topo |
| **R** | Reiniciar a partida |
| **M** | Ligar / desligar música de fundo |
| **ESC** | Sair do jogo |
| **Arrastar mouse** (botão esquerdo) | Orbitar a câmera de 3ª pessoa |

---

## Requisitos

- Compilador C++ com suporte a **C++11** (g++ 4.8+ ou clang++ 3.3+)
- **OpenGL** 1.x / 2.1 — pipeline fixed-function
- **GLUT** — freeglut no Windows/Linux, GLUT.framework no macOS
- **GLU** — utilitários OpenGL (incluso com GLUT na maioria dos ambientes)

Não há dependências externas além das listadas: a biblioteca `stb_image.h` está incluída diretamente no projeto.

---

## Bibliotecas usadas

| Biblioteca | Função no projeto |
|---|---|
| **OpenGL** (`gl.h`) | Pipeline de renderização fixed-function: geometria, texturas, blending |
| **GLU** (`glu.h`) | Projeção (`gluPerspective`), câmera (`gluLookAt`), quadrics (`gluSphere`, `gluCylinder`), mipmaps (`gluBuild2DMipmaps`) |
| **GLUT** (`glut.h`) | Janela, loop de eventos, callbacks de teclado/mouse, primitivas prontas (`glutSolidSphere`, `glutSolidTorus`, `glutWireSphere`), texto bitmap do HUD |
| **stb_image.h** | Carregamento de imagens JPG/PNG para texturas (Sean Barrett — biblioteca de cabeçalho único) |
| **winmm** (Windows) | Reprodução de efeitos sonoros WAV e BGM MP3 via `PlaySoundA` e `mciSendStringA` |
| **afplay** (macOS) | Reprodução de áudio via ferramenta nativa do sistema |

---

## Como compilar e executar

> **Importante:** execute sempre a partir da **pasta do projeto**, pois os caminhos de texturas e sons são relativos (`textures/`, `sounds/`).

### Windows (Falcon C++ / g++ com glut32)

```bat
g++ SnakeJogo.cpp -o ProjetoFinal.exe -std=c++0x -DGLUT_STATIC -static ^
    -lglut32 -lglu32 -lopengl32 -lwinmm -lgdi32
ProjetoFinal.exe
```

### macOS e Linux (via Makefile)

```bash
make        # compila → gera ./ProjetoFinal
make run    # compila (se necessário) e executa
make clean  # remove o binário
```

No macOS, o Xcode Command Line Tools deve estar instalado (`xcode-select --install`).  
No Linux, instale o freeglut: `sudo apt install freeglut3-dev` (Debian/Ubuntu) ou equivalente.

---

## Documentação técnica

A explicação detalhada de cada sistema do jogo está dividida nos seguintes documentos:

| Documento | Conteúdo |
|---|---|
| [Movimentação](docs/movimentacao.md) | Grid lógico, direção da cobra, passo por tempo, input de teclado/mouse e portais |
| [Formas, Texturas e OBJs](docs/formas-texturas-objs.md) | Primitivas OpenGL/GLU, corpo da cobra, skybox, carregamento de texturas e loader de modelos OBJ |
| [Lógica do Jogo](docs/logica-do-jogo.md) | Estado global, spawn de comida e paredes, colisões, pontuação, velocidade e HUD |
| [Iluminação](docs/iluminacao.md) | Configuração da luz, modelo de Gouraud, materiais e onde a iluminação é desativada |
| [Referências](docs/referencias.md) | Fontes de assets (texturas, modelos, sons) e bibliotecas utilizadas |
