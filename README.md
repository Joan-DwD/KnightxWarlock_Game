# WARLOCK ⚔️ KNIGHT
![OpenGL](https://img.shields.io/badge/OpenGL-FFFFFF?style=for-the-badge&logo=opengl) ![C](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white) ![C++](https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white) 
<table>
  <tr>
    <td>
      <img width="1150" alt="image" src="https://github.com/user-attachments/assets/b1e8bc54-8a62-437d-894a-384246f8d89c" />
      <!-- <img width="1154" height="980" alt="image" src="https://github.com/user-attachments/assets/abb2c428-c55b-49c2-b206-38a3f97e9ef1" /> -->
    </td>
    <td>
      <img width="415" alt="KNIGHTxWARLOCKposterRework" src="https://github.com/user-attachments/assets/97139c04-106d-4946-9fe5-6e1bec4d7a0f" />
    </td>
  </tr>
</table>

### 📄 About

*"Then, all I need for my potion to be done is... **princess blood**... That's the last thing I need... "*

WARLOCK X KNIGHT is a short, single-player adventure, in which you will have to solve a series of puzzles designed around the 'character swap' gimmick. 
Switch between the Warlock and the Knight as you traverse the winding castle rooms. Each challenge surpassed brings you one step closer to your objective: to 
reach the princess's quarters and obtain her blood.

### 🎮 Controls:
- `R` to advance dialogue
- `E` to interact with people/objects
- `SPACE` to swap characters

### 📷 Gallery
<p align="center">
    <img width="49%" alt="image1" src="https://github.com/user-attachments/assets/77f3de7a-4c5b-471b-bdc9-f97b8fcfcc0d" />
&nbsp;
    <img width="49%" alt="image2" src="https://github.com/user-attachments/assets/e7ad6860-0323-40d2-85c1-c5278dd5ede5" />
</p>

<p align="center">
    <!-- <img width="49%" alt="image3" src="https://github.com/user-attachments/assets/7523f32b-9e6e-41eb-9f5a-b4efa137a879" /> -->
    <img width="49%" alt="image4" src="https://github.com/user-attachments/assets/625449af-4468-4bc8-9462-5b8f3acf6e3d" />
&nbsp;
    <!-- <img width="49%" alt="image4" src="https://github.com/user-attachments/assets/625449af-4468-4bc8-9462-5b8f3acf6e3d" /> -->
    <img width="49%" alt="image3" src="https://github.com/user-attachments/assets/7523f32b-9e6e-41eb-9f5a-b4efa137a879" />
</p> 

<p align="center">
    <img width="49%" alt="image5" src="https://github.com/user-attachments/assets/9b92b0c2-ee90-4c45-b7d0-67cd1a2418ee" />
&nbsp;
    <img width="49%" alt="image6" src="https://github.com/user-attachments/assets/03ed8ffc-dc59-43ac-9517-44272bbb2b8d" />
</p> 

## 📥  Installation Steps
Copy the repository and open *GameEngine.sln*. In the Visual Studio terminal, run the following commands:

1. ```
   git clone https://github.com/microsoft/vcpkg 
   .\vcpkg\bootstrap-vcpkg.bat
   .\vcpkg\vcpkg install freetype:x86-windows
   .\vcpkg\vcpkg integrate install
   ```
   (to install the first required library, *freetype*)
2. ```
   .\vcpkg\vcpkg install assimp:x86-windows
   .\vcpkg\vcpkg integrate install
   ```
   (to install the second required library, *assimp*)

3.  Run the game by presing the green play button at the top. Enjoy! :D

## 👤 Authors
| <img src="https://avatars.githubusercontent.com/u/209956358?v=4" width="100"> | <img src="https://avatars.githubusercontent.com/u/183308975?v=4" width="100"> | <img src="https://avatars.githubusercontent.com/u/182642157?v=4" width="100">  | <img src="https://avatars.githubusercontent.com/u/209906609?v=4" width="100"> |
| ------------- | ------------- | ------------- | ------------- |
| **Daoudo Mohamed** | **Nedelcu Andreea**  | **Nedelcu Ioana** | **Profir Andrei** |
| - octree based collisions <br> - water physics (gerstner waves) <br> - key/door animations <br> - mouse enabled object interaction (raycasting) <br> - skybox | - text rendering <br> - dialogue engine <br> - mtl file support (custom assimp mesh loader) <br> - texture tiling implementation <br> - texturing <br> - asset and texture curation | - torch object (placement, logic, animation) <br> - lighting system, shaders <br> - room transition animation <br> - character portraits and poster <br> - readme | - puzzle design and implementation <br> - quest progression logic <br> - room layouts, object placements, texturing <br> - custom object drawing function <br> - character and camera movement <br>- story and dialogue <br> - project manager|

## 📜 License
Under exclusive copyright

## 📖 References
- [Text Rendering](https://learnopengl.com/In-Practice/Text-Rendering) by Learn OpenGL
- [Basic Lighting](https://learnopengl.com/Lighting/Basic-Lighting) by Learn OpenGL
- [Model Loading](https://learnopengl.com/Model-Loading/Model) by Learn OpenGL
- [The Asset-Importer-Lib Documentation](https://the-asset-importer-lib-documentation.readthedocs.io/en/latest/index.html)
