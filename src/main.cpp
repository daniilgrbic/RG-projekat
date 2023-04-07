#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <fmt/core.h>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <board.hpp>
#include <lights.hpp>

#include <iostream>
#include <memory>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

void loadPieceModels();

void renderScene(Shader &shader);

// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

// camera
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

std::map<string, std::shared_ptr<Model>> pieceModels;
std::unique_ptr<Model> board;
std::unique_ptr<Model> cube;
Board game;
Camera camera;
vector <PointLight> pointLights;
vector <SpotLight> spotLights;

int main() {

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
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // tell GLFW to capture our mouse

    // draw in wireframe
//    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);

    glDepthFunc(GL_LESS);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // initialize lights
    // -----------------
    pointLights = vector<PointLight>(3, PointLight());
    pointLights[0].position = glm::vec3(+4.0, +4.0, 3.0);
    pointLights[1].position = glm::vec3(-4.0, +4.0, 2.0);
    pointLights[2].position = glm::vec3(4.0, +0.0, 4.0);
    pointLights[2].enabled = true;

    spotLights = vector<SpotLight>(2, SpotLight());
    spotLights[0].position = glm::vec3(+0.0, +2.0, 4.0);
    spotLights[0].direction = normalize(glm::vec3(+0.0, -1.3, -1.0));
    spotLights[0].enabled = true;
    spotLights[1].position = glm::vec3(-8.0, -4.0, 4.0);
    spotLights[1].direction = normalize(glm::vec3(+2.0, +2.0, -1.0));
    spotLights[1].diffuse *= 5.0;
    spotLights[1].enabled = true;

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
    unsigned int depthMapFBOs[pointLights.size()+spotLights.size()];
    unsigned int depthCubemaps[pointLights.size()+spotLights.size()];
    for(unsigned int i = 0; i < pointLights.size()+spotLights.size(); i++) {
        glGenFramebuffers(1, depthMapFBOs + i);
        glGenTextures(1, depthCubemaps + i);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemaps[i]);
        for (unsigned int j = 0; j < 6; ++j) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
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
    loadPieceModels();
    cube = std::make_unique<Model>("resources/objects/cube.obj");

    // initialize board & camera
    // -------------------------
    game = Board();
    camera = Camera(glm::vec3(0.0f, -9.0f, 9.0f));

    while (!glfwWindowShouldClose(window)) {
        auto currentFrame = (float) glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float near_plane = 1.0f;
        float far_plane  = 25.0f;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
        std::vector<glm::mat4> shadowTransforms;

        for(unsigned int i = 0; i < pointLights.size(); i++) {
            // 0. create depth cube map transformation matrices
            // ------------------------------------------------
            shadowTransforms.clear();
            shadowTransforms.push_back(shadowProj * glm::lookAt(pointLights[i].position, pointLights[i].position + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(pointLights[i].position, pointLights[i].position + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(pointLights[i].position, pointLights[i].position + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(pointLights[i].position, pointLights[i].position + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(pointLights[i].position, pointLights[i].position + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(pointLights[i].position, pointLights[i].position + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));

            // 1. render scene to depth cube map
            // ---------------------------------
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

        for(unsigned int i = 0; i < spotLights.size(); i++) {
            // 0. create depth cube map transformation matrices
            // ------------------------------------------------
            shadowTransforms.clear();
            shadowTransforms.push_back(shadowProj * glm::lookAt(spotLights[i].position, spotLights[i].position + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(spotLights[i].position, spotLights[i].position + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(spotLights[i].position, spotLights[i].position + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(spotLights[i].position, spotLights[i].position + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(spotLights[i].position, spotLights[i].position + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(spotLights[i].position, spotLights[i].position + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));

            // 1. render scene to depth cube map
            // ---------------------------------
            glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
            glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBOs[pointLights.size()+i]);
            glClear(GL_DEPTH_BUFFER_BIT);
            depthShader.use();
            for (unsigned int j = 0; j < 6; ++j) {
                depthShader.setMat4("shadowMatrices[" + std::to_string(j) + "]", shadowTransforms[j]);
            }
            depthShader.setFloat("far_plane", far_plane);
            depthShader.setVec3("lightPos", spotLights[i].position);
            renderScene(depthShader);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // 2. render scene as normal
        // -------------------------
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        objectShader.use();
        objectShader.setFloat("far_plane", far_plane);
        objectShader.setVec3("cameraPos", camera.Position);
        for(unsigned int i = 0; i < pointLights.size()+spotLights.size(); i++) {
            objectShader.setInt(fmt::format("depthMaps[{}]", i), 15+(int)i);
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
            objectShader.setBool (fmt::format("pointLights[{}].enabled"  , i), pointLights[i].enabled);
        }

        for(unsigned int i = 0; i < spotLights.size(); i++) {
            objectShader.setVec3 (fmt::format("spotLights[{}].position"   , i), spotLights[i].position);
            objectShader.setVec3 (fmt::format("spotLights[{}].direction"  , i), spotLights[i].direction);
            objectShader.setVec3 (fmt::format("spotLights[{}].ambient"    , i), spotLights[i].ambient);
            objectShader.setVec3 (fmt::format("spotLights[{}].diffuse"    , i), spotLights[i].diffuse);
            objectShader.setVec3 (fmt::format("spotLights[{}].specular"   , i), spotLights[i].specular);
            objectShader.setFloat(fmt::format("spotLights[{}].constant"   , i), spotLights[i].constant);
            objectShader.setFloat(fmt::format("spotLights[{}].linear"     , i), spotLights[i].linear);
            objectShader.setFloat(fmt::format("spotLights[{}].quadratic"  , i), spotLights[i].quadratic);
            objectShader.setFloat(fmt::format("spotLights[{}].cutOff"     , i), spotLights[i].cutOff);
            objectShader.setFloat(fmt::format("spotLights[{}].outerCutOff", i), spotLights[i].outerCutOff);
            objectShader.setBool (fmt::format("spotLights[{}].enabled"    , i), spotLights[i].enabled);
        }

        objectShader.setVec3 ("viewPosition"        , camera.Position);
        objectShader.setFloat("material.shininess"  , 32.0f);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        objectShader.setMat4("projection", projection);
        objectShader.setMat4("view", view);

        renderScene(objectShader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void renderScene(Shader &shader) {
    { // render board
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.06f));
        model = glm::scale(model, glm::vec3(0.183f));
        shader.setMat4("model", model);
        board->Draw(shader);
    }

    { // render chess pieces
        std::vector < std::tuple<float, int, char> > pieces;
        for(int row = 1; row <= 8; row++) {
            for (char col = 'a'; col <= 'h'; col++) {
                pieces.emplace_back(glm::length(game.get_position(row, col) - camera.Position), row, col);
            }
        }
        sort(pieces.rbegin(), pieces.rend());
        for(auto piece : pieces) {
            string piece_name = game.get_piece(std::get<1>(piece), std::get<2>(piece));
            if(!piece_name.empty()) {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, game.get_position(std::get<1>(piece), std::get<2>(piece)));
                if(piece_name == "knight_white")
                    model = glm::translate(model, glm::vec3(0.0, +0.16, 0.0));
                if(piece_name == "knight_black")
                    model = glm::translate(model, glm::vec3(0.0, -0.16, 0.0));
                model = glm::scale(model, glm::vec3(0.183f));
                shader.setMat4("model", model);
                pieceModels[piece_name]->Draw(shader);
            }
        }
    }

    { // render point lights
        for(auto & pointLight : pointLights) {
            if(!pointLight.enabled)
                continue;
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, pointLight.position);
            model = glm::scale(model, glm::vec3(0.1f));
            shader.setMat4("model", model);
            cube->Draw(shader);
        }
    }

    { // render spotlights
        for(auto & spotLight : spotLights) {
            if(!spotLight.enabled)
                continue;
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, spotLight.position);
            model = glm::scale(model, glm::vec3(0.1f));
            shader.setMat4("model", model);
            cube->Draw(shader);
        }
    }
}

void loadPieceModels() {
    string path = "resources/objects/stone_chess/";
    std::vector<string> piece_names = {
            "pawn_white", "rook_white", "knight_white", "bishop_white", "king_white", "queen_white",
            "pawn_black", "rook_black", "knight_black", "bishop_black", "king_black", "queen_black"
    };
    for(const auto& name : piece_names) {
        pieceModels[name] = std::make_shared<Model>(path + name + "/modelf.obj");
    }
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    (void) window;
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    (void) window;
    if (firstMouse) {
        lastX = (float) xpos;
        lastY = (float) ypos;
        firstMouse = false;
    }

    float xoffset = (float) xpos - lastX;
    float yoffset = lastY - (float) ypos; // reversed since y-coordinates go from bottom to top

    lastX = (float) xpos;
    lastY = (float) ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    (void) window;
    (void) xoffset;
    camera.ProcessMouseScroll((float) yoffset);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    (void) window;
    (void) key;
    (void) scancode;
    (void) action;
    (void) mods;
}
