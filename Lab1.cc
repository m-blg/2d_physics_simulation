

#define GUI_ENABLED 1
#include "gpu_graphics/SDL_main.cc"

#include "gpu_graphics/loadings.cc"
#include "gpu_graphics/draw.cc"
#include "physics.cc"

#include "cp_lib/basic.cc"
#include "cp_lib/array.cc"
#include "cp_lib/vector.cc"
#include "cp_lib/memory.cc"
#include "cp_lib/io.cc"


using namespace cp;

struct {
    bool is_physics_updated = true;
    bool are_colliders_rendered = true;
    vec4f clear_color = {0, 0.2, 0.2, 1};
} Sandbox_Settings;

sbuff<vec3f, 4> quad_vrt_positions = {{
    { -0.5f, -0.5f, 0 },
	{ 0.5f, -0.5f, 0 },
	{ 0.5f, 0.5f, 0 },
	{ -0.5f, 0.5f, 0 }
}};

sbuff<u32[3], 10> quad_triangles = {{
    {0, 1, 2}, //face front
    {0, 2, 3}
}};


sbuff<vec2f, 4> quad_uvs = {{
    {0, 0},
    {1, 0},
    {1, 1},
    {0, 1}
}};

Mesh quad_mesh = {
    { begin(&quad_vrt_positions), cap(&quad_vrt_positions) }, 
    { begin(&quad_triangles), cap(&quad_triangles) }
};

u32 quad_vao;
u32 quad_vbo;
u32 quad_ibo;
u32 mvp_mat_loc;

u32 stream_vao;
u32 stream_vbo;

namespace Editor {
    Physics_Object* selected_object = null;

    void place_object();
}

namespace Builder {

    Physics_Object* selected_object = null;

    sarr<Physics_Object, 2> objects = {{
        {
            "Box", { {}, {1, 0, 0, 0}, {1, 1, 1} },
            { .type = Collider_Type::Box_Collider2D, 
            .box_collider2d = {{-0.5f, -0.5f}, {0.5f, 0.5f}}},
            { 1, {}, false },
            { 0, 0, {1, 1, 1, 1}}
        },
        {
            "Shere", { {}, {1, 0, 0, 0}, {1, 1, 1} },
            { .type = Collider_Type::Sphere_Collider2D, 
            .sphere_collider2d = {{}, 0.5f}},
            { 1, {}, false },
            { 0, 1, {1, 1, 1, 1}}
        }
    }};

    void init_objects() {
        objects[0].name = "Box";
        objects[0].transform = { {}, {1, 0, 0, 0}, {1, 1, 1} };
        objects[0].collider = { .type = Collider_Type::Box_Collider2D, 
        .box_collider2d = {{-0.5f, -0.5f}, {0.5f, 0.5f}}};
        objects[0].physics_data = { 1, {}, false };
        objects[0].material = { 0, 0, {1, 1, 1, 1}};
    }
}

struct Camera {
    Transform transform;
    vec2f pixels_per_unit;
};

Camera camera = {
    {
    { 0, 0, 0 },
    { 1, 0, 0, 0},
    { 0.5, 0.5, 0.5 }
    },
    {100, 100}
};

Camera* main_camera;


void save_physics_objects(const char* file_name) {
    FILE* file = fopen(file_name, "wb");
    fwrite(&Sandbox_Settings, sizeof(Sandbox_Settings), 1, file);
    fwrite(&camera, sizeof(Camera), 1, file);
    fwrite(&quads.len, sizeof(u32), 1, file);
    fwrite(quads.buffer, sizeof(Physics_Object), quads.len, file);
    fclose(file);
}

void load_physics_objects(const char* file_name) {
    FILE* file = fopen(file_name, "rb");
    if (file == null) 
        return;

    fread(&Sandbox_Settings, sizeof(Sandbox_Settings), 1, file);
    fread(&camera, sizeof(Camera), 1, file);
    u32 len;
    fread(&len, sizeof(u32), 1, file);
    quads.init(len);
    quads.len = len;
    fread(quads.buffer, sizeof(Physics_Object), len, file);
    fclose(file);
}

void render_quads() {
    bind_shader(Assets::shaders[0].id);

    bind_vao(quad_vao);
    bind_ibo(quad_ibo);

    mat4f vp_m = proj_xy_orth_matrix(window_size, main_camera->pixels_per_unit, {-1, 30}) * view_matrix(&main_camera->transform);

    for (auto it = begin(&quads); it != end(&quads); it++) {
        i32 texture_slot = 0;
        bind_texture(Assets::textures[it->material.texture_name].id, texture_slot);

        set_uniform(&Assets::shaders[0], 1, texture_slot);
        vec4f color = it->material.color;
        set_uniform(&Assets::shaders[0], 2, color);

        mat4f mvp_m = vp_m * model_matrix(&it->transform);
        set_uniform(&Assets::shaders[0], 0, mvp_m);

        glDrawElements(GL_TRIANGLES, cap(&quad_mesh.index_buffer) * 3, GL_UNSIGNED_INT, null);
    }
}

void render_colliders() {
    bind_shader(Assets::shaders[0].id);

    bind_vao(quad_vao);
    bind_ibo(quad_ibo);

    i32 texture_slot = 1;

    set_uniform(&Assets::shaders[0], 1, texture_slot);


    mat4f vp_m = proj_xy_orth_matrix(window_size, main_camera->pixels_per_unit, {-1, 30}) * view_matrix(&main_camera->transform);

    for (auto it = begin(&quads); it != end(&quads); it++) {
        mat4f mvp_m;
        if (it->collider.type == Collider_Type::Box_Collider2D) {
            bind_texture(Assets::textures[2].id, texture_slot);

            Box_Collider2D& bc = it->collider.box_collider2d;
            vec2f collider_size = bc.rt - bc.lb;
            vec2f collider_center = (bc.rt + bc.lb) / 2.0f;
            Transform t = it->transform;
            t.position += vec3f(collider_center.x, collider_center.y, 0);
            t.scale = { t.scale.x * collider_size.x, t.scale.y * collider_size.y, t.scale.z };
            mvp_m = vp_m * model_matrix(&t);
        } else if (it->collider.type == Collider_Type::Sphere_Collider2D) {
            bind_texture(Assets::textures[3].id, texture_slot);

            Sphere_Collider2D& c = it->collider.sphere_collider2d;
            Transform t = it->transform;
            t.position += vec3f(c.origin.x, c.origin.y, 0);
            t.scale = { max(t.scale.x, t.scale.y) * 2 * c.radius, max(t.scale.x, t.scale.y)* 2 * c.radius, t.scale.z };
            mvp_m = vp_m * model_matrix(&t);
        }

        set_uniform(&Assets::shaders[0], 0, mvp_m);
        vec4f color = ( it == Editor::selected_object ? vec4f{ 1, 1, 1, 0.8f } : vec4f{ 1, 1, 1, 0.5f } );
        set_uniform(&Assets::shaders[0], 2, color);

        glDrawElements(GL_TRIANGLES, cap(&quad_mesh.index_buffer) * 3, GL_UNSIGNED_INT, null);
    }
}

void Editor::place_object() {
    vec2f cursor_world_pos = screen_to_world_space(Input::mouse_position, main_camera->transform, window_size, main_camera->pixels_per_unit);
    Editor::selected_object = is_over(cursor_world_pos);
    if (Editor::selected_object != null) return;

    if (Builder::selected_object != null) {
        Physics_Object obj = *Builder::selected_object;
        obj.transform.position = {cursor_world_pos.x, cursor_world_pos.y, 0};

        dpush(&quads, obj);
    }
}

void explosion_effect() {
    const f32 accel_radius = 10;
    const f32 accel_magnitude = 3000;
    vec2f cursor_world_pos = screen_to_world_space(Input::mouse_position, main_camera->transform, window_size, main_camera->pixels_per_unit);
    for (auto it = begin(&quads); it != end(&quads); it++) {
        if (it->physics_data.is_static)
            continue;

        vec2f center;
        if (it->collider.type == Collider_Type::Box_Collider2D) {
            center = centerof(world_space_collider_b(it));                    
        } else {
            center = world_space_collider_s(it).origin;  
        }
        vec2f dir = center - cursor_world_pos;
        if (magnitude(dir) < accel_radius) {
            it->physics_data.velocity += dir / magnitude(dir) * accel_magnitude * GTime::dt;
        }
    }
}


void game_init() {
    Input::input_init();

    Assets::load_shaders<1>({"Assets/Shaders/sprite.glsl"});
    Assets::shaders[0].init(3);
    add_uniform(&Assets::shaders[0], "u_mpv_mat", Type::mat4f);
    add_uniform(&Assets::shaders[0], "u_texture", Type::i32);
    add_uniform(&Assets::shaders[0], "u_color", Type::vec4f);

    Assets::load_textures<4>({
        "Assets/Textures/SquareTexture.png", "Assets/Textures/CircleTexture.png",
        "Assets/Textures/BoxCollider2D.png", "Assets/Textures/SphereCollider2D.png"});


    glGenVertexArrays(1, &quad_vao);
    glBindVertexArray(quad_vao);

    // make buffer for triangle
    glGenBuffers(1, &quad_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);

    // allocates vram
    glBufferData(GL_ARRAY_BUFFER, size(&quad_mesh.vertex_buffer) + sizeof(quad_uvs), null, GL_STATIC_DRAW);
    
    glBufferSubData(GL_ARRAY_BUFFER, 0, size(&quad_mesh.vertex_buffer), begin(&quad_mesh.vertex_buffer));
    glBufferSubData(GL_ARRAY_BUFFER, size(&quad_mesh.vertex_buffer), sizeof(quad_uvs), begin(&quad_uvs));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)size(&quad_mesh.vertex_buffer));

    glGenBuffers(1, &quad_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size(&quad_mesh.index_buffer), begin(&quad_mesh.index_buffer), GL_STATIC_DRAW);


    glUseProgram(Assets::shaders[0].id);
    

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Assets::textures[0].id);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 

    //glEnable(GL_DEPTH_TEST);
    //glDepthFunc(GL_LESS);

    main_camera = &camera;

    load_physics_objects("Saves/save.bin");

    GTime::fixed_dt = 1.0f / 360;
}

void game_shut() {
    Input::input_shut();
}


void game_update() {

    glClearColor(Sandbox_Settings.clear_color.r, Sandbox_Settings.clear_color.g, Sandbox_Settings.clear_color.b, Sandbox_Settings.clear_color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    if (Input::is_key_down('q')) {
        is_running = false;
    }

    if (Input::is_key_held('w')) {
        main_camera->transform.position += vec3f(0, 0.1, 0);
    }
    if (Input::is_key_held('s')) {
        main_camera->transform.position += vec3f(0, -0.1, 0);
    }
    if (Input::is_key_held('a')) {
        main_camera->transform.position += vec3f(-0.1, 0, 0);
    }
    if (Input::is_key_held('d')) {
        main_camera->transform.position += vec3f(0.1, 0, 0);
    }

    ImGuiIO& gui_io = ImGui::GetIO();
    if (!gui_io.WantCaptureMouse) {
        if (Input::is_mouse_button_down(0)) {
            Editor::place_object();
        }
        if (Input::is_mouse_button_down(2)) {
            explosion_effect();
        }

        if (main_camera != null) {
            vec2f v = main_camera->pixels_per_unit;
            f32 val = 1 + Input::mouse_wheel.y / 5.0f;
            main_camera->pixels_per_unit = {v.x * val, v.y * val} ;
        } 
    }

    render_quads();
    if (Sandbox_Settings.are_colliders_rendered)
        render_colliders();

    static f32 cur_dt = 0;
    cur_dt += GTime::dt;
    for (; cur_dt >= GTime::fixed_dt; cur_dt -= GTime::fixed_dt) {
        if (Sandbox_Settings.is_physics_updated)
            physics_update();  
    }

    // cube_transform.position += vec3f(0.5, 0.5, -1);
    // to_mat4(&tr_m, &cube_transform);
    // glUniformMatrix4fv(mvp_mat_loc, 1, GL_TRUE, (f32*)&tr_m);
    // glDrawElements(GL_TRIANGLES, cap(&quad_mesh.index_buffer) * 3, GL_UNSIGNED_INT, null);
    // cube_transform.position -= vec3f(0.5, 0.5, -1);


}

#if GUI_ENABLED
#include "gpu_graphics/import/imgui/imgui_demo.cpp"
void draw_gui() {
    ImGui::Begin("Properties");
    // ImGui::ShowDemoWindow();
    ImGui::Text("fps: %f, dt: %f", 1 / GTime::dt, GTime::dt);

    ImGui::Checkbox("Update Physics", &Sandbox_Settings.is_physics_updated);
    ImGui::Checkbox("Render Colliders", &Sandbox_Settings.are_colliders_rendered);

    if (ImGui::CollapsingHeader("Other")) {
        ImGui::ColorPicker4("Background Color", (f32*)&Sandbox_Settings.clear_color);
    }

    ImGui::Separator();

    ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | 
        ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    
    if (ImGui::TreeNode("Physics Objects")) {
        for (u32 i = 0; i < len(&quads); i++) {
            ImGuiTreeNodeFlags node_flags = base_flags;
            if (&quads[i] == Editor::selected_object) {
                node_flags |= ImGuiTreeNodeFlags_Selected;
            }
            ImGui::TreeNodeEx((void*)(intptr_t)(i32)i, node_flags, quads[i].name);
            if (ImGui::IsItemClicked()) {
                Editor::selected_object = &quads[i];
            }
        }
        ImGui::TreePop();
    }

    // test();

    const f32 abs_max_speed = 300;
    if (Editor::selected_object != null) {
        if (ImGui::CollapsingHeader("Transform")) {
            ImGui::SliderFloat3("Position", (f32*)(&Editor::selected_object->transform.position), -100, 100);
            ImGui::SliderFloat4("Rotation", (f32*)(&Editor::selected_object->transform.rotation), -100, 100);
            ImGui::SliderFloat3("Scale", (f32*)(&Editor::selected_object->transform.scale), -100, 100);
        }
        if (ImGui::CollapsingHeader("Collider")) {
            i32 item_current = (i32)Editor::selected_object->collider.type;
            ImGui::Combo("combo 2 (one-liner)", &item_current, "Box Collider2D\0Sphere Collider2D\0\0");
            Editor::selected_object->collider.type = (Collider_Type)item_current;

            if (Editor::selected_object->collider.type == Collider_Type::Box_Collider2D) {
                Box_Collider2D& collider = Editor::selected_object->collider.box_collider2d;
                ImGui::SliderFloat2("lb", (f32*)(&collider.lb), -100, 100);
                ImGui::SliderFloat2("rt", (f32*)(&collider.rt), -100, 100);
            } else if (Editor::selected_object->collider.type == Collider_Type::Sphere_Collider2D) {
                Sphere_Collider2D& collider = Editor::selected_object->collider.sphere_collider2d;
                ImGui::SliderFloat2("origin", (f32*)(&collider.origin), -100, 100);
                ImGui::SliderFloat("radius", (f32*)(&collider.radius), -100, 100);
            }
        }
        if (ImGui::CollapsingHeader("Material")) {
            Material_Sprite2D& mat = Editor::selected_object->material;
            char combo_lable[10]; sprintf(combo_lable, "%u", mat.shader_name);
            if (ImGui::BeginCombo("Shader", combo_lable, 0))
            {
                for (u32 n = 0; n < SHADER_COUNT; n++)
                {
                    const bool is_selected = (mat.shader_name == n);
                    char name[10]; sprintf(name, "%u", n);
                    if (ImGui::Selectable(name, is_selected))
                        mat.shader_name = n;

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                        strcpy(combo_lable, name);
                    }
                }
                ImGui::EndCombo();
            }

            sprintf(combo_lable, "%u", mat.texture_name);
            if (ImGui::BeginCombo("Texture", combo_lable, 0))
            {
                for (u32 n = 0; n < TEXTURE_COUNT; n++)
                {
                    const bool is_selected = (mat.shader_name == n);
                    char name[10]; sprintf(name, "%u", n);
                    if (ImGui::Selectable(name, is_selected))
                        mat.texture_name = n;

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::ColorPicker4("Color", (f32*)&mat.color);
            
        }

        if (ImGui::CollapsingHeader("Physcics Data")) {
            Physics_Data& data = Editor::selected_object->physics_data;
            ImGui::SliderFloat("Mass", (f32*)(&data.mass), 0, 100);
            ImGui::SliderFloat2("Velocity", (f32*)(&data.velocity), -abs_max_speed, abs_max_speed);
            ImGui::Checkbox("Is Static", &data.is_static);
        }
        if (ImGui::Button("Delete")) {
            remove(&quads, Editor::selected_object);
            Editor::selected_object = null;
        }
    }


    static char save_file_name_buffer[100] = "save.bin";
    if (ImGui::Button("Save")) {
        char s[110] = "Saves/";
        strcat(s, save_file_name_buffer);
        save_physics_objects(s);
    }
    ImGui::SameLine();
    ImGui::InputText("Save file name", save_file_name_buffer, 100);

    static char load_file_name_buffer[100] = "save.bin";
    if (ImGui::Button("Load")) {
        char s[110] = "Saves/";
        strcat(s, load_file_name_buffer);
        load_physics_objects(s);
    }
    ImGui::SameLine();
    ImGui::InputText("Load file name", load_file_name_buffer, 100);

    ImGui::End();

    ImGui::Begin("Build Menu");
    if (ImGui::TreeNode("Physics Objects")) {
        {
            ImGuiTreeNodeFlags node_flags = base_flags;
            if (Builder::selected_object == null) {
                node_flags |= ImGuiTreeNodeFlags_Selected;
            }
            ImGui::TreeNodeEx((void*)(intptr_t)(i32)(-1), node_flags, "None");
            if (ImGui::IsItemClicked()) {
                Builder::selected_object = null;
            }
        }
        for (u32 i = 0; i < cap(&Builder::objects); i++) {
            ImGuiTreeNodeFlags node_flags = base_flags;
            if (&Builder::objects[i] == Builder::selected_object) {
                node_flags |= ImGuiTreeNodeFlags_Selected;
            }
            ImGui::Image((void*)(intptr_t)Assets::textures[Builder::objects[i].material.texture_name].id, ImVec2(30, 30));
            ImGui::SameLine();
            ImGui::TreeNodeEx((void*)(intptr_t)(i32)i, node_flags, Builder::objects[i].name);
            if (ImGui::IsItemClicked()) {
                Builder::selected_object = &Builder::objects[i];
            }

        }
        ImGui::TreePop();
    }
    ImGui::End();

}
#else

void draw_gui() {}

#endif

