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
};

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    float shininess;
};

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

#define POINT_LIGHTS 1
uniform PointLight pointLights[POINT_LIGHTS];

uniform Material material;
uniform vec3 viewPosition;

uniform float far_plane;
uniform samplerCube depthMap;

vec3 globalAmbient = vec3(0.0);

float ShadowCalculation(vec3 fragPos)
{
    // get vector between fragment position and light position
    vec3 fragToLight = fragPos - pointLights[0].position;
    // ise the fragment to light vector to sample from the depth map
    float closestDepth = texture(depthMap, fragToLight).r;
    // it is currently in linear range between [0,1], let's re-transform it back to original depth value
    closestDepth *= far_plane;
    // now get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);
    // test for shadows
    float bias = 0.05; // we use a much larger bias since depth is now in [near_plane, far_plane] range

    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    // display closestDepth as debug (to visualize depth cubemap)
    //FragColor = vec4(vec3(closestDepth / far_plane), 1.0);
    //FragColor = vec4(vec3(1.0-shadow), 1.0);

    return shadow;
}

// calculates the color when using a point light.
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
    vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords));
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    globalAmbient += ambient;

    return (diffuse + specular);
}

void main()
{
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(viewPosition - FragPos);

    float shadow = ShadowCalculation(FragPos);

    vec3 result = vec3(0);
    for(int i = 0; i < POINT_LIGHTS; i++) {
        result += CalcPointLight(pointLights[i], normal, FragPos, viewDir);
    }

    FragColor = vec4(globalAmbient+result*(1.0-shadow), 1.0);
}
