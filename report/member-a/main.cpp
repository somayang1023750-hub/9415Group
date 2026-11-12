// Report-only staging. All models, impacts, fire and UI use production code.
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include "graphics/OpenGL.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <stb_image_write.h>
#include "app/AppUi.h"
#include "app/GameplayUi.h"
#include "core/AssetPath.h"
#include "core/gameplay/ParticleEffects.h"
#include "graphics/ArenaRenderer.h"
#include "graphics/Shader.h"
#include "graphics/Window.h"
#include "imgui.h"
#include "world/ArenaWorld.h"
using namespace craft;

static gameplay::Gameplay uiFixture(int mode, const world::ArenaWorld& world) {
  gameplay::Gameplay game(5);
  const glm::vec3 player(16, gameplay::kActorCentreY, 16);
  if (mode == 1) {
    // Reach the real upgrade phase through kills and EXP collection.
    for (int k=0;k<3;++k) {
      const int kills=game.stats().kills;
      if (!game.spawnEnemy({16,1.45F,20.5F},player,world,gameplay::EnemyKind::Spider))
        throw std::runtime_error("Cannot prepare upgrade fixture");
      for(int f=0;f<500 && game.stats().kills==kills;++f)
        game.update(.01F,player,world,game.enemies().front().position);
      if (game.orbs().empty()) throw std::runtime_error("Fixture did not drop EXP");
      const auto pickup=game.orbs().front().position;
      game.update(.01F,pickup,world);
    }
  } else if (mode==2) {
    for(int f=0;f<1810;++f) game.update(.1F,player,world);
  } else {
    game.spawnEnemy({16,1.45F,22},player,world);
    for(int f=0;f<300 && game.stats().phase==gameplay::Phase::Playing;++f)
      game.update(.1F,player,world);
  }
  const auto expected=mode==1?gameplay::Phase::ChoosingUpgrade:mode==2?gameplay::Phase::Won:gameplay::Phase::Lost;
  if(game.stats().phase!=expected)throw std::runtime_error("UI fixture failed to reach real phase");
  return game;
}
static void capture(int w,int h,int figure) {
  if(w<=0 || h<=0) return;
  std::vector<unsigned char> pixels(static_cast<std::size_t>(w)*h*3);
  glPixelStorei(GL_PACK_ALIGNMENT,1);
  glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
  stbi_flip_vertically_on_write(1);
  std::filesystem::create_directories("member-a-captures");
  const auto file="member-a-captures/figure-"+std::to_string(figure)+"-"+
    std::to_string(static_cast<long long>(glfwGetTime()*1000))+".png";
  if(!stbi_write_png(file.c_str(),w,h,3,pixels.data(),w*3))
    throw std::runtime_error("PNG capture failed");
  std::cout<<"Saved "<<file<<'\n';
}
int main(int argc,char** argv) try {
  const bool smoke=argc>1 && std::string(argv[1])=="--smoke";
  graphics::Window window(1280,720,"Member A - production model/effect showcase");
  graphics::Shader shader(core::assetPath("shaders/basic.vert"),core::assetPath("shaders/basic.frag"));
  world::ArenaLayout layout; // No autonomous spawning or gameplay difficulty changes.
  world::ArenaWorld world(layout);
  graphics::ArenaRenderer renderer(world,core::assetPath("textures/player.png"),
    core::assetPath("textures/zombie.png"),core::assetPath("textures/sand_noise_uv.png"));
  app::AppUi ui(window); ImGui::GetIO().IniFilename=nullptr;
  gameplay::ParticleEffects fx;
  auto preview=uiFixture(1,world);
  int figure=1,uiMode=1,frames=0; bool night=false,paused=false,controls=true;
  float clock=0,modelYaw=.35F; double last=glfwGetTime(); bool wasCapture=false,wasHide=false; int lastKey=0;
  glEnable(GL_DEPTH_TEST);
  while(!window.shouldClose()) {
    window.pollEvents();const auto now=glfwGetTime();
    const float dt=std::min(.05F,static_cast<float>(now-last));last=now;
    if(smoke) { figure=1+frames/32; night=frames%32>=16; if(frames%32==0){clock=0;fx.reset();} }
    const bool hide=glfwGetKey(window.nativeHandle(),GLFW_KEY_H)==GLFW_PRESS;
    if(hide && !wasHide) controls=!controls; wasHide=hide;
    int key=0;
    for(int i=0;i<4;++i)if(glfwGetKey(window.nativeHandle(),GLFW_KEY_F1+i)==GLFW_PRESS)key=1+i;
    if(glfwGetKey(window.nativeHandle(),GLFW_KEY_U)==GLFW_PRESS)key=5;
    if(glfwGetKey(window.nativeHandle(),GLFW_KEY_V)==GLFW_PRESS)key=6;
    if(glfwGetKey(window.nativeHandle(),GLFW_KEY_D)==GLFW_PRESS)key=7;
    if(key && key!=lastKey) {
      if(key<=4)figure=key;
      else {figure=2;uiMode=key-4;preview=uiFixture(uiMode,world);}
      clock=0;fx.reset();
    }
    lastKey=key;
    bool step=false,freezeHit=false;
    ui.beginFrame();
    // Leave real modal pages cleanly when switching back to a capture scene.
    if(figure!=2)for(const char* name:{"LEVEL UP - Choose an upgrade","VICTORY - Arena survived","DEFEAT - Run ended"})
      if(ImGui::IsPopupOpen(name) && ImGui::BeginPopupModal(name)) {
        ImGui::CloseCurrentPopup();ImGui::EndPopup();
      }
    if(controls) {
      ImGui::Begin("Report capture controls");
      ImGui::TextUnformatted("Staged evidence; production rendering and effect parameters.");
      for(int i=1;i<=4;++i) {
        if(i>1) ImGui::SameLine();
        const auto name="Figure "+std::to_string(i);
        if(ImGui::Button(name.c_str())) { figure=i;clock=0;fx.reset(); }
      }
      ImGui::Checkbox("Night lighting",&night);ImGui::SameLine();ImGui::Checkbox("Pause",&paused);
      ImGui::SliderFloat("Model yaw",&modelYaw,-3.14F,3.14F);
      if(ImGui::Button("Replay effects")) { clock=0;fx.reset();step=true; }
      ImGui::SameLine(); if(ImGui::Button("Step 0.02 s")){paused=true;step=true;}
      if(ImGui::Button("Freeze hit for Figure 3")){figure=3;clock=0;fx.reset();paused=true;step=true;freezeHit=true;}
      if(figure==2) {
        const char* names[]={"Upgrade","Victory","Defeat"};
        for(int i=1;i<=3;++i) { if(i>1)ImGui::SameLine(); if(ImGui::Button(names[i-1])) { uiMode=i;preview=uiFixture(i,world); } }
      }
      ImGui::TextUnformatted("F1-F4: figure | U/V/D: upgrade/win/loss | H: hide | F12: PNG");
      ImGui::End();
    }
    auto size=window.framebufferSize();
    if(size.first<=0 || size.second<=0) { ui.render();window.swapBuffers();continue; }
    const glm::vec3 centre(16,1.3F,16),eye=centre+glm::vec3(0,6,9);
    const float aspect=static_cast<float>(size.first)/size.second;
    const float half=figure==1?2.3F:4.6F;
    const auto vp=glm::ortho(-half*aspect,half*aspect,-half,half,.1F,70.F)*
      glm::lookAt(eye,centre,glm::vec3(0,1,0));
    float previous=clock;
    const float tick=smoke?.04F:step?.02F:dt;
    if(!paused || step)clock+=tick;
    if(clock>=4.2F) {clock=0;previous=-1;fx.reset();}
    std::vector<gameplay::ImpactEvent> events;
    std::vector<gameplay::BurningArea> areas;
    if(figure==3 && previous<=0 && clock>0)
      for(int i=0;i<3;++i) events.push_back({centre+glm::vec3((i-1)*3.F,0,0),{1,0,0},
        i==0?gameplay::Weapon::Bullet:gameplay::Weapon::Laser,i==2});
    const auto bombProfile=gameplay::weaponProfile(gameplay::Weapon::Bomb,4);
    const float flight=.7F;
    if(figure==4 && clock>=flight) {
      if(previous<flight)events.push_back({centre,{0,1,0},gameplay::Weapon::Bomb,true,bombProfile.blastRadius});
      if(clock-flight<3) areas.push_back({centre,bombProfile.blastRadius,clock-flight});
    }
    if(!paused || step)fx.update(tick,events,areas);
    if(freezeHit){fx.update(.05F,{},areas);clock+=.05F;}
    glViewport(0,0,size.first,size.second);glClearColor(.06F,.07F,.09F,1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    renderer.setEffectLights(shader,night,fx.lights());
    renderer.drawWorld(shader,vp,eye,night?world::ArenaLevel::Night:world::ArenaLevel::Day,centre,true);
    if(figure==1) {
      std::vector<graphics::ArenaRenderer::ZombieInstance> models;
      for(int i=0;i<3;++i) {
        const auto kind=static_cast<gameplay::EnemyKind>(i);
        models.push_back({{16+(i-1)*2.6F,gameplay::enemyProfile(kind).centreY,16},modelYaw,0,false,-1,kind});
      }
      renderer.drawZombies(shader,vp,models);
    }
    if(figure==4 && clock<flight) {
      gameplay::Bullet bomb{centre+glm::vec3((clock-flight)*bombProfile.bulletSpeed,0,0),{bombProfile.bulletSpeed,0,0},3-clock,bombProfile.baseDamage,gameplay::Weapon::Bomb};
      bomb.evolved=true;
      renderer.drawProxy(shader,vp,bomb.position,glm::vec3(.22F),{1,.12F,.06F},1.5708F,true);
      renderer.drawFirebombTrail(shader,vp,bomb); // Exact same helper as ArenaScene.
    }
    for(const auto& a:areas)renderer.drawBlast(shader,vp,a.position,a.radius,0);
    for(const auto& p:fx.groundFire())renderer.drawProxy(shader,vp,p.position,p.halfExtents,glm::vec3(p.color));
    renderer.drawParticles(shader,vp,fx.visuals());
    auto label=[&](glm::vec3 p,const char* text) {
      auto clip=vp*glm::vec4(p,1);clip/=clip.w;
      auto width=ImGui::CalcTextSize(text).x;
      ImGui::GetBackgroundDrawList()->AddText({(clip.x+1)*size.first*.5F-width*.5F,(1-clip.y)*size.second*.5F},IM_COL32_WHITE,text);
    };
    if(figure==1) {label(centre+glm::vec3(-2.6F,-.6F,1),"Zombie");label(centre+glm::vec3(0,-.6F,1),"Golem");label(centre+glm::vec3(2.6F,-.6F,1),"Spider");}
    if(figure==3) {label(centre+glm::vec3(-3,-.5F,1),"Bullet debris");label(centre+glm::vec3(0,-.5F,1),"Laser: cyan");label(centre+glm::vec3(3,-.5F,1),"Evolved Laser: red");}
    if(smoke && figure==2 && frames%32==12)preview=uiFixture(2,world);
    if(smoke && figure==2 && frames%32==24)preview=uiFixture(3,world);
    if(figure==2 && app::drawGameplayUi(preview))preview=uiFixture(uiMode,world);
    ui.render();
    const bool shot=glfwGetKey(window.nativeHandle(),GLFW_KEY_F12)==GLFW_PRESS;
    if((shot && !wasCapture) || (smoke && (frames%32==2 || frames%32==14 || frames%32==26))) capture(size.first,size.second,figure);
    wasCapture=shot;
    if(glGetError()!=GL_NO_ERROR)throw std::runtime_error("OpenGL error");
    window.swapBuffers();
    if(smoke && ++frames>=128)window.requestClose();
  }
  return 0;
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
