#version 410 core
out vec4 FragColor;

struct PointLight {
    vec3 position;

    vec3 specular;
    vec3 diffuse;
    vec3 ambient;

    float constant;
    float linear;
    float quadratic;

    bool enabled;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    bool enabled;
};

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

#define NUM_POINT_LIGHTS 1
#define NUM_SPOTLIGHTS 3
uniform PointLight pointLights[NUM_POINT_LIGHTS];
uniform SpotLight spotLights[NUM_SPOTLIGHTS];

uniform Material material;
uniform vec3 viewPosition;

uniform float far_plane;
uniform samplerCube depthMaps[NUM_POINT_LIGHTS+NUM_SPOTLIGHTS];

uniform vec3 cameraPos;

vec3 globalAmbient = vec3(0.0);

// function prototypes
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
float ShadowCalculation(vec3 fragPos, int depthMapId, vec3 lightPos);

void main()
{
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(viewPosition - FragPos);
    vec3 combined = vec3(0.0);
    for(int i = 0; i < NUM_POINT_LIGHTS; i++) {
        if(!pointLights[i].enabled)
            continue;
        vec3 color = CalcPointLight(pointLights[i], normal, FragPos, viewDir);
        float shadow = ShadowCalculation(FragPos, i, pointLights[i].position);
        combined += (1.0-shadow)*color;
    }
    for(int i = 0; i < NUM_SPOTLIGHTS; i++) {
        if(!spotLights[i].enabled)
            continue;
        vec3 color = CalcSpotLight(spotLights[i], normal, FragPos, viewDir);
        float shadow = ShadowCalculation(FragPos, NUM_POINT_LIGHTS+i, spotLights[i].position);
        combined += (1.0-shadow)*color;
    }
    float distanceToCamera = length(FragPos-cameraPos) / 1.5;
    if (distanceToCamera > 1.0)
            distanceToCamera = 1.0;
    FragColor = vec4(globalAmbient+combined, distanceToCamera);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);

    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);

    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    //float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);

    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    globalAmbient += ambient;
    return (diffuse + specular);
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    ambient *= attenuation;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;

    globalAmbient += ambient;
    return (diffuse + specular);
}

float ShadowCalculation(vec3 fragPos, int depthMapId, vec3 lightPos)
{
    vec3 fragToLight = fragPos - lightPos;
    float shadow = 0.0;
    float bias = 0.05;
    float samples = 4.0;
    float offset = 0.25;
    for(float x = -offset; x < offset; x += offset / (samples * 0.5)) {
     for(float y = -offset; y < offset; y += offset / (samples * 0.5)) {
         for(float z = -offset; z < offset; z += offset / (samples * 0.5)) {
             float closestDepth = texture(depthMaps[depthMapId], fragToLight + vec3(x, y, z) * 0.05).r; // use lightdir to lookup cubemap
             closestDepth *= far_plane;
             if(length(fragToLight) - bias > closestDepth)
             shadow += 1.0;
         }
     }
    }
    shadow /= (samples * samples * samples);
    return shadow;
}
