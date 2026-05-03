#version 330 core
out vec4 FragColor;
in vec2 TexCoords;
in vec3 FragPos;
in mat3 TBN;
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
const float metallic = 0.0;
const float roughness = 0.6;
const float PI = 3.14159265359;
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness*roughness; float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0); float NdotH2 = NdotH*NdotH;
    return a2 / (PI * pow(NdotH2*(a2-1.0)+1.0, 2.0));
}
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness+1.0; float k = r*r/8.0;
    return NdotV / (NdotV*(1.0-k)+k);
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    return GeometrySchlickGGX(max(dot(N,V),0.0),roughness)
         * GeometrySchlickGGX(max(dot(N,L),0.0),roughness);
}
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0-F0)*pow(clamp(1.0-cosTheta,0.0,1.0),5.0);
}
void main() {
    vec3 albedo = pow(texture(texture_diffuse1, TexCoords).rgb, vec3(2.2));
    if (length(albedo) < 0.05) albedo = vec3(0.5);
    vec3 normalMap = texture(texture_normal1, TexCoords).rgb;
    vec3 N;
    if (length(normalMap) < 0.05) {
        N = normalize(TBN[2]);
    } else {
        N = normalize(normalMap * 2.0 - 1.0);
        N = normalize(TBN * N);
    }
    vec3 V = normalize(viewPos - FragPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 L = normalize(lightPos - FragPos);
    vec3 H = normalize(V + L);
    float distance = length(lightPos - FragPos);
    vec3 radiance = lightColor * (1.0/(distance*distance)) * 10000.0;
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
    vec3 kD   = (vec3(1.0) - F) * (1.0 - metallic);
    vec3 specular = (NDF*G*F) / (4.0*max(dot(N,V),0.0)*max(dot(N,L),0.0)+0.0001);
    float NdotL = max(dot(N, L), 0.0);
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;
    vec3 ambient = vec3(0.2) * albedo;
    vec3 color = ambient + Lo;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));
    FragColor = vec4(color, 1.0);
}