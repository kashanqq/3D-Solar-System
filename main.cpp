#include <iostream>
#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <Model.h>
#include <Shader.h>

// Global Variables

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 20.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

int WINDOW_WIDTH = 1280;
int WINDOW_HEIGHT = 720;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float yaw = 0.0f;
float pitch = 0.0f;
float lastX = (float)WINDOW_WIDTH / 2;
float lastY = (float)WINDOW_HEIGHT / 2;

bool firstMouse = true;

// End global variables

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// Inputs
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	
	float cameraSpeed = 10.0f * deltaTime;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		cameraPos += cameraSpeed * cameraFront;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		cameraPos -= cameraSpeed * cameraFront;
	}

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	}

}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse) {
		lastX = (float)xpos;
		lastY = (float)ypos;
		firstMouse = false;
	}
	float xoffset = (float)xpos - lastX;
	float yoffset = lastY - (float)ypos; // reversed since y-coordinates go from bottom to top
	lastX = (float)xpos;
	lastY = (float)ypos;
	float sensitivity = 0.1f; // change this value to your liking
	xoffset *= sensitivity;
	yoffset *= sensitivity;
	yaw += xoffset;
	pitch += yoffset;
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;
	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(front);
}



int main() {

	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;
	}


	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "3d solar system", NULL, NULL);

	if (!window)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
	}

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, mouse_callback);

	glEnable(GL_DEPTH_TEST);

	const char* vertextShaderSrc = 
		"#version 330 core\n"
		"layout (location = 0) in vec3 aPos;\n"
		"layout (location = 1) in vec3 aNormal;\n"
		"layout (location = 2) in vec2 aTexCoords;\n"
		"layout (location = 3) in vec3 aTangent;\n"

		"out vec2 TexCoords;\n"
		"out vec3 FragPos;\n"
		"out mat3 TBN;\n"

		"uniform mat4 model;\n"
		"uniform mat4 view;\n"
		"uniform mat4 projection;\n"

		"void main()\n"
		"{\n"
		"   FragPos = vec3(model * vec4(aPos, 1.0));\n"
		"   TexCoords = aTexCoords;\n"

		"   vec3 T = normalize(vec3(model * vec4(aTangent, 0.0)));\n"
		"   vec3 N = normalize(vec3(model * vec4(aNormal, 0.0)));\n"
		"   vec3 B = cross(N, T);\n"
		"   TBN = mat3(T, B, N);\n"

		"   gl_Position = projection * view * vec4(FragPos, 1.0);\n"
		"}\0";
		

	const char* fragmentShaderSrc = 
		"#version 330 core\n"
		"out vec4 FragColor;\n"

		"in vec2 TexCoords;\n"
		"in vec3 FragPos;\n"
		"in mat3 TBN;\n" // Принимаем матрицу из вершинного шейдера

		// Наши текстуры
		"uniform sampler2D texture_diffuse1;\n"
		"uniform sampler2D texture_normal1;\n"

		// Параметры сцены
		"uniform vec3 lightPos;\n"
		"uniform vec3 viewPos;\n"
		"uniform vec3 lightColor;\n"

		// Глобальные константы PBR (пока задаем жестко для дерева/камня)
		"const float metallic = 0.0;\n" // 0 - диэлектрик (дерево, пластик), 1 - металл
		"const float roughness = 0.6;\n" // 0 - зеркало, 1 - мел/штукатурка
		"const float PI = 3.14159265359;\n"

		// --- Функции PBR ---
		// 1. Распределение микрограней GGX
		"float DistributionGGX(vec3 N, vec3 H, float roughness) {\n"
		"    float a = roughness*roughness;\n"
		"    float a2 = a*a;\n"
		"    float NdotH = max(dot(N, H), 0.0);\n"
		"    float NdotH2 = NdotH*NdotH;\n"
		"    float num = a2;\n"
		"    float denom = (NdotH2 * (a2 - 1.0) + 1.0);\n"
		"    denom = PI * denom * denom;\n"
		"    return num / denom;\n"
		"}\n"

		// 2. Геометрическое затенение Смита
		"float GeometrySchlickGGX(float NdotV, float roughness) {\n"
		"    float r = (roughness + 1.0);\n"
		"    float k = (r*r) / 8.0;\n"
		"    float num = NdotV;\n"
		"    float denom = NdotV * (1.0 - k) + k;\n"
		"    return num / denom;\n"
		"}\n"
		"float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {\n"
		"    float NdotV = max(dot(N, V), 0.0);\n"
		"    float NdotL = max(dot(N, L), 0.0);\n"
		"    float ggx2 = GeometrySchlickGGX(NdotV, roughness);\n"
		"    float ggx1 = GeometrySchlickGGX(NdotL, roughness);\n"
		"    return ggx1 * ggx2;\n"
		"}\n"

		// 3. Уравнение Френеля (Шлик)
		"vec3 fresnelSchlick(float cosTheta, vec3 F0) {\n"
		"    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);\n"
		"}\n"

		// --- Главная функция ---
		"void main() {\n"
	// 1. Читаем базовый цвет
		"    vec3 albedo = pow(texture(texture_diffuse1, TexCoords).rgb, vec3(2.2));\n"

		// ЗАПАСНОЙ ПЛАН ДЛЯ ЦВЕТА: Если текстуры нет, делаем серый пластик/металл
		"    if (length(albedo) < 0.05) {\n"
		"       albedo = vec3(0.5);\n" // Можешь поменять 0.5 на любой другой цвет краски
		"    }\n"

		// 2. Читаем карту нормалей
		"    vec3 normalMap = texture(texture_normal1, TexCoords).rgb;\n"
		"    vec3 N;\n"

		// ЗАПАСНОЙ ПЛАН ДЛЯ НОРМАЛЕЙ: Если карты рельефа нет (черный цвет)
		"    if (length(normalMap) < 0.05) {\n"
		// Берем оригинальную гладкую нормаль из 3-го столбца матрицы TBN
		"        N = normalize(TBN[2]);\n"
		"    } else {\n"
		// Если карта есть - считаем рельеф
		"        N = normalize(normalMap * 2.0 - 1.0);\n"
		"        N = normalize(TBN * N);\n"
		"    }\n"

		"    vec3 V = normalize(viewPos - FragPos);\n"

		// F0 - базовое отражение поверхности
		// Для не-металлов это обычно 0.04. Для металлов - это сам цвет albedo.
		"    vec3 F0 = vec3(0.04);\n"
		"    F0 = mix(F0, albedo, metallic);\n"

		// Настраиваем источник света
		"    vec3 L = normalize(lightPos - FragPos);\n"
		"    vec3 H = normalize(V + L);\n"
		"    float distance = length(lightPos - FragPos);\n"
		"    float attenuation = 1.0 / (distance * distance);\n" // Затухание света по закону обратных квадратов
		"    vec3 radiance = lightColor * attenuation * 10000.0;\n" // Интенсивность лампочки

		// Считаем Cook-Torrance BRDF
		"    float NDF = DistributionGGX(N, H, roughness);\n"
		"    float G   = GeometrySmith(N, V, L, roughness);\n"
		"    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);\n"

		// Находим баланс между отраженным светом (kS) и поглощенным/рассеянным (kD)
		"    vec3 kS = F;\n"
		"    vec3 kD = vec3(1.0) - kS;\n"
		"    kD *= 1.0 - metallic;\n"

		"    vec3 numerator    = NDF * G * F;\n"
		"    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;\n"
		"    vec3 specular     = numerator / denominator;\n"

		// Итоговый свет от лампочки
		"    float NdotL = max(dot(N, L), 0.0);\n"
		"    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;\n"

		// Фоновый свет (ambient), чтобы в тени не было черноты
		"    vec3 ambient = vec3(0.1) * albedo;\n"
		"    vec3 color = ambient + Lo;\n"

		// HDR тонемаппинг (Reinhard) и гамма-коррекция
		"    color = color / (color + vec3(1.0));\n"
		"    color = pow(color, vec3(1.0/2.2));\n"

		"    FragColor = vec4(color, 1.0);\n"
		"}\0";

	Shader ourShader(vertextShaderSrc, fragmentShaderSrc);

	Model planet("models/car.obj");


	while (!glfwWindowShouldClose(window))
	{

		float currentTime = (float)glfwGetTime();
		deltaTime = currentTime - lastFrame;
		lastFrame = currentTime;


		processInput(window);
		/* Render here */
		glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ourShader.use();

		ourShader.setVec3("viewPos", cameraPos);

		glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
		ourShader.setMat4("projection", projection);
		ourShader.setMat4("view", view);

		glm::vec3 lightColor = glm::vec3(1.0f, 0.9f, 0.8f);


		float time = (float)glfwGetTime();
		glm::vec3 lightPos = glm::vec3(sin(time) * 10.0f, 10.0f ,cos(time) * 10.0f);
		
		ourShader.setVec3("lightPos", lightPos);
		ourShader.setVec3("viewPos", cameraPos);
		ourShader.setVec3("lightColor", lightColor);

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
		ourShader.setMat4("model", model);

		planet.Draw(ourShader);

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	glfwTerminate();


	return 0;
}