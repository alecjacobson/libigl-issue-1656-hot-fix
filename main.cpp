///////////////////////////////////////////////////////////////////////////////
// pending bug fix in libigl
#include <imgui/imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "igl/opengl/glfw/imgui/ext/ImGuizmoPlugin.h"
#include "igl/opengl/glfw/imgui/ext/ImGuiMenu.h"
// These are derived from true igl ImGuiMenu (not ext::)
namespace igl{ namespace opengl{ namespace glfw{ namespace imgui{
class PrePlugin: public igl::opengl::glfw::imgui::ImGuiMenu
{
public:
  PrePlugin(){};
  IGL_INLINE virtual bool pre_draw() override { ImGuiMenu::pre_draw(); return false;}
  IGL_INLINE virtual bool post_draw() override { return false;}
};
class PostPlugin: public igl::opengl::glfw::imgui::ImGuiMenu
{
public:
  PostPlugin(){};
  IGL_INLINE virtual bool pre_draw() override { return false;}
  IGL_INLINE virtual bool post_draw() override {  ImGui::Render(); ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); return false;}
};
}}}}
///////////////////////////////////////////////////////////////////////////////


#include <igl/read_triangle_mesh.h>
#include <igl/opengl/glfw/Viewer.h>

int main(int argc, char * argv[])
{
  Eigen::MatrixXd V;
  Eigen::MatrixXi F;
  igl::read_triangle_mesh(argv[1],V,F);
  igl::opengl::glfw::Viewer vr;

  igl::opengl::glfw::imgui::ext::ImGuizmoPlugin imguizmo;
  igl::opengl::glfw::imgui::ext::ImGuiMenu menu;
  igl::opengl::glfw::imgui::PrePlugin PRE;
  igl::opengl::glfw::imgui::PostPlugin POST;
  vr.plugins.push_back(&PRE);
  vr.plugins.push_back(&menu);
  vr.plugins.push_back(&imguizmo);
  vr.plugins.push_back(&POST);

  vr.data().set_mesh(V,F);
  
  // Initialize ImGuizmo at mesh centroid
  imguizmo.T.block(0,3,3,1) = 
    0.5*(V.colwise().maxCoeff() + V.colwise().minCoeff()).transpose().cast<float>();
  // Update can be applied relative to this remembered initial transform
  const Eigen::Matrix4f T0 = imguizmo.T;
  // Attach callback to apply imguizmo's transform to mesh
  imguizmo.callback = [&](const Eigen::Matrix4f & T)
  {
    const Eigen::Matrix4d TT = (T*T0.inverse()).cast<double>().transpose();
    vr.data().set_vertices(
      (V.rowwise().homogeneous()*TT).rowwise().hnormalized());
    vr.data().compute_normals();
  };
  // Maya-style keyboard shortcuts for operation
  vr.callback_key_pressed = [&](decltype(vr) &,unsigned int key, int mod)
  {
    switch(key)
    {
      case ' ': imguizmo.visible = !imguizmo.visible; return true;
      case 'W': case 'w': imguizmo.operation = ImGuizmo::TRANSLATE; return true;
      case 'E': case 'e': imguizmo.operation = ImGuizmo::ROTATE;    return true;
      case 'R': case 'r': imguizmo.operation = ImGuizmo::SCALE;     return true;
    }
    return false;
  };
  std::cout<<R"(
W,w  Switch to translate operation
E,e  Switch to rotate operation
R,r  Switch to scale operation
)";
  vr.launch();
}
