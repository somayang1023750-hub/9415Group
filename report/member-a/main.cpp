// Interactive training range. Real ArenaScene updates/rendering and collision.
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "graphics/OpenGL.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <stb_image_write.h>
#include "app/AppUi.h"
#include "app/ArenaScene.h"
#include "core/AssetPath.h"
#include "graphics/Shader.h"
#include "graphics/Window.h"
#include "imgui.h"
using namespace craft;

static void prepare(app::ArenaScene& scene) {
  world::ArenaLayout layout;
  layout.playerSpawn={16,1.45F,18.5F};
  layout.obstacles={{12,13,1,11,2},{21,14,1,6,2},{15,12,4,1,2},{17,21,3,1,2}};
  scene.world()=world::ArenaWorld(layout);scene.rebuildWorldGeometry();
  scene.gameplay().setShowcaseEnabled(true);scene.resetGameplay();
  const std::array<glm::vec3,5> points={glm::vec3(13.5F,1.45F,13.8F),
    glm::vec3(13.5F,1.45F,22),glm::vec3(16,1.45F,13.3F),
    glm::vec3(1.3F,1.45F,7),glm::vec3(30.6F,1.45F,25)};
  const std::array<gameplay::EnemyKind,5> kinds={gameplay::EnemyKind::Zombie,
    gameplay::EnemyKind::Golem,gameplay::EnemyKind::Spider,
    gameplay::EnemyKind::Zombie,gameplay::EnemyKind::Golem};
  for(std::size_t i=0;i<points.size();++i)
    if(!scene.gameplay().spawnEnemy(points[i],scene.player().position(),scene.world(),kinds[i]))
      throw std::runtime_error("Training target is not clear of geometry/player");
}
static void capture(int w,int h) {
  if(w<=0 || h<=0) return;
  std::vector<unsigned char> pixels(static_cast<std::size_t>(w)*h*3);
  glPixelStorei(GL_PACK_ALIGNMENT,1);
  glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
  stbi_flip_vertically_on_write(1);
  std::filesystem::create_directories("member-a-captures");
  const auto file="member-a-captures/range-"+
    std::to_string(static_cast<long long>(glfwGetTime()*1000))+".png";
  if(!stbi_write_png(file.c_str(),w,h,3,pixels.data(),w*3))
    throw std::runtime_error("PNG capture failed");
  std::cout<<"Saved "<<file<<'\n';
}

int main(int argc,char** argv) try {
  const bool smoke=argc>1 && std::string(argv[1])=="--smoke";
  graphics::Window window(1280,720,"Member A - interactive training range");
  graphics::Shader shader(core::assetPath("shaders/basic.vert"),core::assetPath("shaders/basic.frag"));
  app::ArenaScene scene(core::assetPath("textures/player.png"),core::assetPath("textures/zombie.png"),core::assetPath("textures/sand_noise_uv.png"));
  prepare(scene);const auto initial=scene.gameplay().enemies();
  app::AppUi ui(window);ImGui::GetIO().IniFilename=nullptr;
  bool paused=false,autoPause=false,controls=true,night=false;
  std::array<bool,3> owned={true,false,false},evolved={false,false,false};
  std::array<int,3> upgrades={0,0,0};int selected=0,frames=0;
  std::array<bool,GLFW_KEY_LAST+1> oldKeys{};
  auto pressed=[&](int key) {bool down=glfwGetKey(window.nativeHandle(),key)==GLFW_PRESS;
    bool edge=down&&!oldKeys[key];oldKeys[key]=down;return edge;};
  auto apply=[&](int i) {if(!scene.gameplay().configureShowcaseWeapon(static_cast<gameplay::Weapon>(i),owned[i],upgrades[i],evolved[i]))
    throw std::runtime_error("Invalid range weapon setting");};
  glEnable(GL_DEPTH_TEST);double previous=glfwGetTime();
  float frozenTime=-1;bool sawImpact=false,sawFire=false;
  std::vector<gameplay::Bullet> frozenBullets;std::vector<gameplay::BurningArea> frozenFire;
  while(!window.shouldClose()) {
    window.pollEvents();const double now=glfwGetTime();
    const float dt=std::clamp(static_cast<float>(now-previous),0.F,.1F);previous=now;
    ui.beginFrame();
    if(pressed(GLFW_KEY_SPACE))paused=!paused;
    if(pressed(GLFW_KEY_H))controls=!controls;
    if(pressed(GLFW_KEY_F))autoPause=!autoPause;
    bool step=pressed(GLFW_KEY_N)&&paused;
    for(int i=0;i<3;++i)if(pressed(GLFW_KEY_1+i)) {
      selected=i;for(int j=0;j<3;++j){owned[j]=j==i;apply(j);}
    }
    if(pressed(GLFW_KEY_U)) {owned[selected]=true;if(upgrades[selected]<4)++upgrades[selected];else evolved[selected]=true;apply(selected);}
    if(pressed(GLFW_KEY_E)) {owned[selected]=true;upgrades[selected]=4;evolved[selected]=true;apply(selected);}
    if(controls) {
      ImGui::Begin("Training range");
      ImGui::TextUnformatted("WASD move | Mouse aim | Hold left mouse to fire");
      ImGui::TextUnformatted("SPACE: time stop/resume | N: advance 1/60 second");
      ImGui::TextUnformatted("1/2/3: solo weapon | U: upgrade | E: evolve");
      ImGui::Text("Selected weapon: %s",gameplay::weaponName(static_cast<gameplay::Weapon>(selected)));
      ImGui::Checkbox("Time stopped",&paused);
      if(ImGui::Button("Step 1/60 s")){paused=true;step=true;}
      ImGui::Checkbox("Auto-stop on actual impact (F)",&autoPause);
      if(ImGui::Checkbox("Night lighting",&night))
        scene.setShowcaseLighting(night?world::ArenaLevel::Night:world::ArenaLevel::Day);
      for(int i=0;i<3;++i) {
        ImGui::PushID(i);ImGui::Separator();
        ImGui::TextUnformatted(gameplay::weaponName(static_cast<gameplay::Weapon>(i)));
        bool changed=ImGui::Checkbox("Owned / fire",&owned[i]);
        if(ImGui::SliderInt("Upgrade stage",&upgrades[i],0,4)){evolved[i]=false;changed=true;}
        if(ImGui::Checkbox("Evolved",&evolved[i])){if(evolved[i])upgrades[i]=4;changed=true;}
        if(changed)apply(i);ImGui::PopID();
      }
      if(ImGui::Button("Reset range (clear all effects)")) {
        prepare(scene);owned={true,false,false};upgrades={0,0,0};evolved={false,false,false};frozenTime=-1;
      }
      ImGui::TextUnformatted("Five stationary, invulnerable targets. No waves or countdown.");
      ImGui::TextUnformatted("H: hide controls | F12: PNG | ESC: exit");ImGui::End();
    }
    const auto size=window.framebufferSize();
    if(size.first<=0||size.second<=0){ui.render();window.swapBuffers();continue;}
    if(smoke) {
      autoPause=frames<110;
      if(frames==110){paused=false;owned={false,false,true};upgrades={0,0,4};evolved={false,false,true};for(int i=0;i<3;++i)apply(i);}
      if(frames==145)paused=true;
      if(frames==160)paused=false;
    }
    if(!paused || step) {
      const float tick=step?1.F/60.F:smoke?.02F:dt;
      if(smoke)scene.update({},tick,initial[frames<110?0:1].position);
      else {
        app::MovementInput input{};
        if(!ImGui::GetIO().WantCaptureKeyboard)input={glfwGetKey(window.nativeHandle(),GLFW_KEY_W)==GLFW_PRESS,
          glfwGetKey(window.nativeHandle(),GLFW_KEY_S)==GLFW_PRESS,glfwGetKey(window.nativeHandle(),GLFW_KEY_A)==GLFW_PRESS,
          glfwGetKey(window.nativeHandle(),GLFW_KEY_D)==GLFW_PRESS};
        if(glfwGetMouseButton(window.nativeHandle(),GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS && !ImGui::GetIO().WantCaptureMouse) {
          double x,y;int w,h;glfwGetCursorPos(window.nativeHandle(),&x,&y);glfwGetWindowSize(window.nativeHandle(),&w,&h);
          scene.updateFromMouse(input,tick,{x,y,w,h,size.first,size.second});
        } else scene.update(input,tick);
      }
      if(!scene.gameplay().impacts().empty()) {sawImpact=true;if(autoPause)paused=true;}
    }
    if(smoke) {
      const auto& game=scene.gameplay();
      if(game.enemies().size()!=initial.size())throw std::runtime_error("Target count changed");
      for(std::size_t i=0;i<initial.size();++i)
        if(game.enemies()[i].position!=initial[i].position || game.enemies()[i].health!=initial[i].health)
          throw std::runtime_error("Target moved or lost health");
      if(paused && !step) {
        if(frozenTime>=0) {
          if(game.elapsedSeconds()!=frozenTime || game.bullets().size()!=frozenBullets.size() || game.burningAreas().size()!=frozenFire.size())
            throw std::runtime_error("Frozen simulation changed");
          for(std::size_t i=0;i<frozenBullets.size();++i)
            if(game.bullets()[i].position!=frozenBullets[i].position || game.bullets()[i].life!=frozenBullets[i].life)
              throw std::runtime_error("Frozen projectile changed");
          for(std::size_t i=0;i<frozenFire.size();++i)
            if(game.burningAreas()[i].age!=frozenFire[i].age)throw std::runtime_error("Frozen fire advanced");
        }
        frozenTime=game.elapsedSeconds();frozenBullets=game.bullets();frozenFire=game.burningAreas();
      } else frozenTime=-1;
      sawFire|=!game.burningAreas().empty();
    }
    glViewport(0,0,size.first,size.second);glClearColor(.06F,.07F,.09F,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    scene.render(shader,size.first,size.second,true);
    if(paused)ImGui::GetForegroundDrawList()->AddText({size.first*.5F-50,16},IM_COL32(255,220,80,255),"TIME STOPPED");
    ui.render();
    if(pressed(GLFW_KEY_F12) || (smoke&&(frames==80||frames==145||frames==150)))capture(size.first,size.second);
    if(glGetError()!=GL_NO_ERROR)throw std::runtime_error("OpenGL error");
    window.swapBuffers();
    if(smoke&&++frames>=240){if(!sawImpact||!sawFire)throw std::runtime_error("Smoke missed collision/fire");window.requestClose();}
  }
  if(smoke)std::cout<<"PASS: real contacts, invulnerable stationary targets, frozen projectiles/fire and resume\n";
  return 0;
} catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
