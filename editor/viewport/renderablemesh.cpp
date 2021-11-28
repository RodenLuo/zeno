#include "renderablemesh.h"
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <zeno/math/vec.h>
#include <zeno/types/Mesh.h>
#include <zeno/types/MeshTriangulate.h>

ZENO_NAMESPACE_BEGIN

namespace {

class RenderableMesh final : public Renderable
{
    static std::unique_ptr<QOpenGLShaderProgram> makeShaderProgram() {
        auto program = std::make_unique<QOpenGLShaderProgram>();
        program->addShaderFromSourceCode(QOpenGLShader::Vertex, R"(
uniform mat4 uMVP;

attribute vec3 attrPos;
varying vec3 varyPos;

void main() {
    varyPos = attrPos;
    gl_Position = uMVP * vec4(attrPos, 1.0);
}
)");
        program->addShaderFromSourceCode(QOpenGLShader::Fragment, R"(
uniform mat4 uMVP;
uniform mat4 uInvMVP;
uniform mat4 uInvMV;

varying vec3 varyPos;

vec3 pbr(vec3 albedo, float roughness, float metallic, float specular,
    vec3 nrm, vec3 idir, vec3 odir) {

  vec3 hdir = normalize(idir + odir);
  float NoH = max(0., dot(hdir, nrm));
  float NoL = max(0., dot(idir, nrm));
  float NoV = max(0., dot(odir, nrm));
  float VoH = clamp(dot(odir, hdir), 0., 1.);
  float LoH = clamp(dot(idir, hdir), 0., 1.);

  vec3 f0 = metallic * albedo + (1. - metallic) * 0.16 * specular * specular;
  vec3 fdf = f0 + (1. - f0) * pow(1. - VoH, 5.);

  float k = (roughness + 1.) * (roughness + 1.) / 8.;
  float vdf = 0.25 / ((NoV * k + 1. - k) * (NoL * k + 1. - k));

  float alpha2 = max(0., roughness * roughness);
  float denom = 1. - NoH * NoH * (1. - alpha2);
  float ndf = alpha2 / (denom * denom);

  vec3 brdf = fdf * vdf * ndf * f0 + (1. - f0) * albedo;
  return brdf * NoL;
}

vec3 calc_ray_dir(vec3 pos) {
    vec4 vpos = uMVP * vec4(pos, 1.);
    vec2 uv = vpos.xy / vpos.w;
    vec4 ro = uInvMVP * vec4(uv, -1., 1.);
    vec4 re = uInvMVP * vec4(uv, +1., 1.);
    vec3 rd = normalize(re.xyz / re.w - ro.xyz / ro.w);
    return rd;
}

vec3 studio_shading(vec3 view_dir, vec3 normal) {
    vec3 matColor = vec3(0.96);
    vec3 color = vec3(0.0);
    vec3 light_dir;

    light_dir = normalize((uInvMV * vec4(1., 2., 5., 0.)).xyz);
    color += vec3(0.45, 0.47, 0.5) * pbr(matColor, 0.19, 0.0, 1.0, normal, light_dir, view_dir);

    light_dir = normalize((uInvMV * vec4(-4., -2., 1., 0.)).xyz);
    color += vec3(0.3, 0.23, 0.18) * pbr(matColor, 0.14, 0.0, 1.0, normal, light_dir, view_dir);

    light_dir = normalize((uInvMV * vec4(3., -5., 2., 0.)).xyz);
    color += vec3(0.15, 0.2, 0.22) * pbr(matColor, 0.23, 0.0, 1.0, normal, light_dir, view_dir);

    color *= 1.2;
    //color = pow(clamp(color, 0., 1.), vec3(1./2.2));
    return color;
}

void main() {
    vec3 view_dir = -calc_ray_dir(varyPos);
    vec3 normal = normalize(cross(dFdx(varyPos), dFdy(varyPos)));
    vec3 color = studio_shading(view_dir, normal);
    gl_FragColor = vec4(color, 1.0);
}
)");
        program->link();
        return program;
    }

public:
    virtual ~RenderableMesh() = default;

    QOpenGLBuffer attrPos;
    size_t mCount{};

    RenderableMesh(std::shared_ptr<types::Mesh> const &mesh)
    {
        auto dataPos = types::meshToTriangles(*mesh);
        mCount = dataPos.size();
        if (!mCount) return;

        attrPos.create();
        attrPos.setUsagePattern(QOpenGLBuffer::StreamDraw);
        attrPos.bind();
        attrPos.allocate(dataPos.data(), dataPos.size() * sizeof(dataPos[0]));
        attrPos.release();
    }

    virtual void render(QDMOpenGLViewport *viewport) override
    {
        if (!mCount) return;

        static auto program = makeShaderProgram();
        program->bind();

        auto view = viewport->getCamera()->getView();
        auto proj = viewport->getCamera()->getProjection();
        auto mv = view;
        auto mvp = proj * mv;
        program->setUniformValue("uMVP", mvp);
        program->setUniformValue("uInvMVP", mvp.inverted());
        program->setUniformValue("uInvMV", mv.inverted());

        attrPos.bind();
        program->enableAttributeArray("attrPos");
        program->setAttributeBuffer("attrPos", GL_FLOAT, 0, 3);
        attrPos.release();

        viewport->glDrawArrays(GL_TRIANGLES, 0, mCount);

        program->disableAttributeArray("attrPos");

        program->release();
    }
};

}

std::unique_ptr<Renderable> makeRenderableMesh(std::shared_ptr<types::Mesh> const &mesh)
{
    return std::make_unique<RenderableMesh>(mesh);
}

ZENO_NAMESPACE_END