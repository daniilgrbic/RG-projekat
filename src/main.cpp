#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fmt/core.h>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <board.hpp>

#include <iostream>
#include <memory>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

void load_stone_chess_models();

void renderScene(Shader shader);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;

    glm::vec3 specular;
    glm::vec3 diffuse;
    glm::vec3 ambient;

    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct DirLight {
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 backpackPosition = glm::vec3(0.0f);
    float backpackScale = 1.0f;
    PointLight pointLight;
    ProgramState() : camera(glm::vec3(0.0f, -9.0f, 9.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

std::map<string, std::shared_ptr<Model>> pieceModels;
std::unique_ptr<Model> board;
std::unique_ptr<Model> cube;
Board game;
//PointLight pointLight;
vector <PointLight> pointLights;
vector <SpotLight> spotLights;

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    //stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    //programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    for(unsigned int i = 0; i < 2; i++) {
        pointLights.emplace_back();
        pointLights[i].ambient = glm::vec3(0.1f);
        pointLights[i].diffuse = glm::vec3(0.6f);
        pointLights[i].specular = glm::vec3(1.0f);
        pointLights[i].constant = 1.0f;
        pointLights[i].linear = 0.09f;
        pointLights[i].quadratic = 0.032f;
    }
    pointLights[0].position = glm::vec3(+4.0, +4.0, 3.0);
    pointLights[1].position = glm::vec3(-4.0, +4.0, 2.0);

    spotLights.emplace_back();
    spotLights[0].position = glm::vec3(0.0, 2.0, 4.0);
    spotLights[0].direction = normalize(glm::vec3(+0.0, -1.0, -1.0));
    spotLights[0].ambient = glm::vec3(0.0);
    spotLights[0].diffuse = glm::vec3(1.0);
    spotLights[0].specular = glm::vec3(1.0);
    spotLights[0].constant = 1.0;
    spotLights[0].linear = 0.09;
    spotLights[0].quadratic = 0.032;
    spotLights[0].cutOff = glm::cos(glm::radians(5.5f));
    spotLights[0].outerCutOff = glm::cos(glm::radians(11.0f));

    // build and compile shaders
    // -------------------------
    Shader objectShader(
        "resources/shaders/object.vert",
        "resources/shaders/object.frag"
    );
    Shader depthShader(
          "resources/shaders/3.2.1.point_shadows_depth.vs",
        "resources/shaders/3.2.1.point_shadows_depth.fs",
        "resources/shaders/3.2.1.point_shadows_depth.gs"
    );

    // configure depth map FBO
    // -----------------------
    const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
    unsigned int depthMapFBOs[pointLights.size()];
    unsigned int depthCubemaps[pointLights.size()];
    for(unsigned int i = 0; i < pointLights.size(); i++) {
        glGenFramebuffers(1, depthMapFBOs + i);
        glGenTextures(1, depthCubemaps + i);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemaps[i]);
        for (unsigned int j = 0; j < 6; ++j) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        // attach depth texture as FBO's depth buffer
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBOs[i]);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemaps[i], 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // load models
    // -----------
    board = std::make_unique<Model>("resources/objects/stone_board/model.obj");
    load_stone_chess_models();
    cube = std::make_unique<Model>("resources/objects/cube.obj");

    // initialize the game
    // -------------------
    game = Board();

    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float near_plane = 1.0f;
        float far_plane  = 25.0f;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
        std::vector<glm::mat4> shadowTransforms;

        for(unsigned int i = 0; i < pointLights.size(); i++) {
            // 0. create depth cube map transformation matrices
            // -----------------------------------------------
            shadowTransforms.clear();
            shadowTransforms.push_back(shadowProj * glm::lookAt(pointLights[i].position, pointLights[i].position + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(pointLights[i].position, pointLights[i].position + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(pointLights[i].position, pointLights[i].position + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(pointLights[i].position, pointLights[i].position + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(pointLights[i].position, pointLights[i].position + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(pointLights[i].position, pointLights[i].position + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));

            // 1. render scene to depth cube map
            // --------------------------------
            glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
            glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBOs[i]);
            glClear(GL_DEPTH_BUFFER_BIT);
            depthShader.use();
            for (unsigned int j = 0; j < 6; ++j) {
                depthShader.setMat4("shadowMatrices[" + std::to_string(j) + "]", shadowTransforms[j]);
            }
            depthShader.setFloat("far_plane", far_plane);
            depthShader.setVec3("lightPos", pointLights[i].position);
            renderScene(depthShader);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // 2. render scene as normal
        // -------------------------
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        objectShader.use();
        for(unsigned int i = 0; i < pointLights.size(); i++) {
            objectShader.setInt(fmt::format("depthMaps[{}]", i), 15+(int)i);
        }
        objectShader.setFloat("far_plane", far_plane);
        for(unsigned int i = 0; i < pointLights.size(); i++) {
            glActiveTexture(GL_TEXTURE15+i);
            glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemaps[i]);
        }

        for(unsigned int i = 0; i < pointLights.size(); i++) {
            objectShader.setVec3 (fmt::format("pointLights[{}].position" , i), pointLights[i].position);
            objectShader.setVec3 (fmt::format("pointLights[{}].ambient"  , i), pointLights[i].ambient);
            objectShader.setVec3 (fmt::format("pointLights[{}].diffuse"  , i), pointLights[i].diffuse);
            objectShader.setVec3 (fmt::format("pointLights[{}].specular" , i), pointLights[i].specular);
            objectShader.setFloat(fmt::format("pointLights[{}].constant" , i), pointLights[i].constant);
            objectShader.setFloat(fmt::format("pointLights[{}].linear"   , i), pointLights[i].linear);
            objectShader.setFloat(fmt::format("pointLights[{}].quadratic", i), pointLights[i].quadratic);
        }

        objectShader.setVec3 (fmt::format("spotLights[{}].position"   , 0), spotLights[0].position);
        objectShader.setVec3 (fmt::format("spotLights[{}].direction"  , 0), spotLights[0].direction);
        objectShader.setVec3 (fmt::format("spotLights[{}].ambient"    , 0), spotLights[0].ambient);
        objectShader.setVec3 (fmt::format("spotLights[{}].diffuse"    , 0), spotLights[0].diffuse);
        objectShader.setVec3 (fmt::format("spotLights[{}].specular"   , 0), spotLights[0].specular);
        objectShader.setFloat(fmt::format("spotLights[{}].constant"   , 0), spotLights[0].constant);
        objectShader.setFloat(fmt::format("spotLights[{}].linear"     , 0), spotLights[0].linear);
        objectShader.setFloat(fmt::format("spotLights[{}].quadratic"  , 0), spotLights[0].quadratic);
        objectShader.setFloat(fmt::format("spotLights[{}].cutOff"     , 0), spotLights[0].cutOff);
        objectShader.setFloat(fmt::format("spotLights[{}].outerCutOff", 0), spotLights[0].outerCutOff);

        objectShader.setVec3 ("viewPosition"        , programState->camera.Position);
        objectShader.setFloat("material.shininess"  , 32.0f);

        // view / projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        objectShader.setMat4("projection", projection);
        objectShader.setMat4("view", view);

        renderScene(objectShader);

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    //programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

void renderScene(Shader shader) {
    { // render board
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.06f));
        model = glm::scale(model, glm::vec3(0.183f));
        shader.setMat4("model", model);
        board->Draw(shader);
    }

    { // render chess pieces
        for(int row = 1; row <= 8; row++) {
            for(char col = 'a'; col <= 'h'; col++) {
                string piece = game.get_piece(row, col);
                if(!piece.empty()) {
                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, game.get_position(row, col));
                    model = glm::scale(model, glm::vec3(0.183f));
                    shader.setMat4("model", model);
                    pieceModels[piece]->Draw(shader);
                }
            }
        }
    }

    { // render point lights
        for(auto & pointLight : pointLights) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, pointLight.position);
            model = glm::scale(model, glm::vec3(0.1f));
            shader.setMat4("model", model);
            cube->Draw(shader);
        }
    }

    { // render spotlights
        for(auto & spotLight : spotLights) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, spotLight.position);
            model = glm::scale(model, glm::vec3(0.1f));
            shader.setMat4("model", model);
            cube->Draw(shader);
        }
    }
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(DOWN, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Backpack position", (float*)&programState->backpackPosition);
        ImGui::DragFloat("Backpack scale", &programState->backpackScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

void load_stone_chess_models() {
    string path = "resources/objects/stone_chess/";
    std::vector<string> piece_names = {
        "pawn_white", "rook_white", "knight_white", "bishop_white", "king_white", "queen_white",
        "pawn_black", "rook_black", "knight_black", "bishop_black", "king_black", "queen_black"
    };
    for(const auto& name : piece_names) {
        pieceModels[name] = std::make_shared<Model>(path + name + "/modelf.obj");
    }
}
