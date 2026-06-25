# Referências

## Assets utilizados no projeto

### Texturas

| Asset | Fonte | Licença | Arquivos no projeto |
|---|---|---|---|
| **Grama** (Grass005) | [ambientCG](https://ambientcg.com) | CC0 (domínio público) | `textures/grass/` |
| **Pavimento** (PavingStones138) | [ambientCG](https://ambientcg.com) | CC0 (domínio público) | `textures/wall/` |
| **Escamas da cobra** (Stylized_Scales_003) | [3DTextures.me](https://3dtextures.me) | CC0 (domínio público) | `textures/snake/Stylized_Scales_003_*` |

### Modelos 3D

| Asset | Fonte | Licença | Arquivos no projeto |
|---|---|---|---|
| **Maçã** (food_apple_01) | [Poly Haven](https://polyhaven.com) | CC0 (domínio público) | `textures/apple/` |
| **Cabeça da cobra** (snake_head) | — | — | `textures/snake/snake_head.obj` |

> O modelo da maçã foi exportado para Blender 3.6 antes de ser incluído no projeto, conforme indicado no arquivo `food_apple_01_4k.mtl`.

### Skybox

| Asset | Autor | Licença | Arquivos no projeto |
|---|---|---|---|
| **Skybox** | Emil Persson (Humus) — [humus.name](http://www.humus.name) | [CC BY 3.0](https://creativecommons.org/licenses/by/3.0/) | `textures/skybox/*.jpg` |

### Áudio

| Asset | Fonte | Arquivo no projeto |
|---|---|---|
| **Música de fundo** | [Pixabay](https://pixabay.com/music/electro-lab-%E3%81%98%E3%81%A3%E3%81%91%E3%82%93%E3%81%97%E3%81%A4-523664/) | `sounds/bgm.mp3` |
| **Efeitos sonoros** (comer, portal, parede, game over) | — | `sounds/*.wav` |

---

## Bibliotecas e ferramentas

### Renderização

| Biblioteca | Versão/Variante | Link |
|---|---|---|
| **OpenGL** | Fixed-function pipeline (1.x / 2.1) | [khronos.org/opengl](https://www.khronos.org/opengl/) |
| **GLU** (OpenGL Utility Library) | Inclusa com OpenGL | [khronos.org](https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/) |
| **GLUT / freeglut** | glut32 (Windows) / GLUT.framework (macOS) / freeglut (Linux) | [freeglut.sourceforge.net](https://freeglut.sourceforge.net) |

### Carregamento de imagens

| Biblioteca | Autor | Link |
|---|---|---|
| **stb_image.h** | Sean T. Barrett | [github.com/nothings/stb](https://github.com/nothings/stb) |

Biblioteca de cabeçalho único (single-header) em domínio público, incluída diretamente como `stb_image.h` na raiz do projeto. Suporta JPEG, PNG, BMP, TGA, GIF, PSD, PNM e outros formatos.

---

## Material de apoio (leitura de referência)

Os recursos abaixo foram usados como consulta durante o desenvolvimento, mas não são fontes de código copiado:

- **NeHe OpenGL Tutorials** — [nehe.gamedev.net](https://nehe.gamedev.net)  
  Referência clássica de OpenGL fixed-function: texturas, iluminação, carregamento de OBJ, quadrics.

- **LearnOpenGL** — [learnopengl.com](https://learnopengl.com)  
  Consulta para conceitos de iluminação de Phong/Gouraud e mipmapping.

- **Documentação OpenGL 2.1** — [khronos.org/registry/OpenGL-Refpages/gl2.1](https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/)  
  Referência das funções `glLight*`, `glColorMaterial`, `gluBuild2DMipmaps`, etc.

- **Documentação freeglut** — [freeglut.sourceforge.net/docs](https://freeglut.sourceforge.net/docs/api.php)  
  Referência de callbacks (`glutTimerFunc`, `glutMouseFunc`, `glutMotionFunc`) e primitivas.

---

## Atribuição do skybox

Conforme exigido pela licença CC BY 3.0 do skybox:

> Skybox por **Emil Persson** (Humus) — http://www.humus.name  
> Licença: Creative Commons Attribution 3.0 Unported — http://creativecommons.org/licenses/by/3.0/
