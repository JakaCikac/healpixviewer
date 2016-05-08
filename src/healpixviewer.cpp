#define GLFW_INCLUDE_GLCOREARB

#include <healpixviewer.h>

using namespace glm;
using namespace std;

static int mousex = 0, mousey = 0, scrolly = 0;

static hpint64 interleave(hpint64 x, hpint64 y)
{
    unsigned char ibit = 0;
    hpint64 ipix = 0;

    while (x || y)
    {
        ipix |= (x & 1) << ibit;
        x >>= 1;
        ibit ++;
        ipix |= (y & 1) << ibit;
        y >>= 1;
        ibit ++;
    }

    return ipix;
}

static float squaref(float x)
{
    return x * x;
}

static void xy2zphi(float x, float y, float *z, float *phi)
{
    float abs_y = fabsf(y);
    if (abs_y <= 0.25)
    {
        *phi = M_PI * x;
        *z = 8. / 3 * y;
    } else {
        if (abs_y == 0.5)
        {
            *phi = 0;
        } else {
            *phi = M_PI * (x - (abs_y - 0.25) / (abs_y - 0.5)
                * (fmodf(x, 0.5) - 0.25));
        }
        *z = copysignf(1 - squaref(2 - 4 * abs_y) / 3, y);
    }
}

// Exit application when Escape key is pressed. Close window, clean exit."
static void keyboard(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

// Mouse cursor position callback. Rotate the sphere.
static void motion(GLFWwindow *window, double x, double y)
{
    mousex = x;
    mousey = y;
}

// Mouse scroll callback. Zoom in and zoom out.
static void scroll(GLFWwindow *window, double xoffset, double yoffset)
{
    scrolly += yoffset;
    if (scrolly < -100)
        scrolly = -100;
    else if (scrolly > 100)
        scrolly = 100;
}

static void error(int error, const char *description)
{
    fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

// convert to NEST if neccessary
float * convert2NEST(float * hp, char ordering, hpint64 npix, long nside) {

    if (ordering == 'R') /* Ring indexing */
    {
        float *new_hp = (float *) malloc(npix * sizeof(*new_hp));
        if (!new_hp)
        {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        hpint64 ipix_nest;
        for (ipix_nest = 0; ipix_nest < npix; ipix_nest ++)
        {
            hpint64 ipix_ring;
            nest2ring64(nside, ipix_nest, &ipix_ring);
            new_hp[ipix_nest] = hp[ipix_ring];
        }
        free(hp);
        hp = new_hp;
    }

    return hp;
}

float * convert2LOG(float *hp, hpint64 npix) {

    float *log_hp = (float *) malloc(npix * sizeof(*log_hp));
        for (int i = 0; i < npix; i++) {
            log_hp[i] = log(hp[i]);
        }

    free(hp);
    hp = log_hp;

    return hp;

}


int main(int argc, char **argv)
{
    // Initialise filename string.
    string fitsfilename{ };
    // Determines if the map is displayed in linear or logarithmic scale.
    bool logarithmic_scale{false};

    /* Validate input */
    if (argc < 2)
    {
        // basic usage
        cout << "Usage: healpixviewer INPUT.fits[.gz] [optional]" << endl;
        // optional parameters
        cout << "Optional parameters: " << endl;
        cout << "   --l  Enable logarithimic scale map display." << endl;

        exit(1);

    } else {

        // Read in the filename. (The map).
        fitsfilename = argv[1];

        if (argc > 2) // if there are optional parameters

            for (int i = 3; i <= argc; i++) {

                // Check for optional parameters.
                if (string(argv[i-1]) == "--l") {
                    // We know the next argument *should* be the filename:
                    logarithmic_scale = true;

                } else if (string(argv[i-1]) == "-d") {
                    // Do nothing yet.
                } else {
                    cout << "Not enough or invalid arguments, please try again.\n";
                    exit(1);

                }
            }
    }

    long nside;
    char coordsys[80];
    char ordering[80];

    /* Read sky map from FITS file */
    float *hp = read_healpix_map(
        fitsfilename.c_str(), &nside, coordsys, ordering);
    if (!hp)
    {
        fputs("error: read_healpix_map\n", stderr);
        exit(EXIT_FAILURE);
    }

    /* Determine total number of pixels */
    hpint64 npix = nside2npix64(nside);

    /* Convert to NEST ordering if necessary */
    hp = convert2NEST(hp, ordering[0], npix, nside);

    /* Convert to Logarithmic scale data if given --l option. */
    if (logarithmic_scale)
        hp = convert2LOG(hp, npix);

    /* Rearrange into base tiles */
    {
        unsigned char ibase;
        float *tile = (float *)malloc(nside * nside * sizeof(*hp));
        if (!tile)
        {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        for (ibase = 0; ibase < 12; ibase ++)
        {
            float *base = hp + nside * nside * ibase;
            long x, y;
            for (x = 0; x < nside; x ++)
                for (y = 0; y < nside; y ++)
                    tile[x * nside + y] = base[interleave(y, x)];
            memcpy(base, tile, nside * nside * sizeof(*tile));
        }
        free(tile);
    }

    /* Rescale to range [0, 255] */
    GLushort *pix = (GLushort *) malloc(npix * sizeof(GLushort));
    if (!pix)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    {
        hpint64 i;
        float max = hp[0];
        // Find maximum value in hp
        for (i = 1; i < npix; i ++)
            if (hp[i] > max)
                max = hp[i];
        // Rescale all values to range [0, 255]
        for (i = 0; i < npix; i ++)
            pix[i] = hp[i] / max * 65535;
    }
    free(hp);

    /* Initialize window system */
    glfwSetErrorCallback(error);
    if (!glfwInit())
        exit(EXIT_FAILURE);

    /* Necessary to get modern OpenGL context on Mavericks */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 1);
    glfwWindowHint(GLFW_SAMPLES, 4);    /* 4x antialiasing */

    GLFWwindow *window = glfwCreateWindow(1024, 768, "HEALPix Viewer", NULL, NULL);
    if ( window == NULL )
    {
        // Log error message.
        std::cerr << "Failed to open GLFW window." << std::endl;
        // Terminate GLFW and exit.
        glfwTerminate();
        exit(EXIT_FAILURE);

    }

    glfwMakeContextCurrent(window);

    /* Load textures */
    GLuint textures[13];
    glGenTextures(13, textures);
    {
        /* Load textures for faces */
        unsigned char ibase;
        for (ibase = 0; ibase < 12; ibase ++)
        {
            glBindTexture(GL_TEXTURE_2D, textures[ibase]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, nside, nside, 0, GL_RED,
                GL_UNSIGNED_SHORT, pix + nside * nside * ibase);
        }
    
        /* Load texture for color map, make active in slot 1 */
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_1D, textures[ibase]);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, sizeof(colormap)/4, 0, GL_RGBA,
            GL_UNSIGNED_BYTE, colormap);
    }
    free(pix);


    // Compile and load vertex shaders and fragment shaders from GLSL source in header file.
    GLuint shader_program = LoadShaders(vertex_shader_source, fragment_shader_source);
    // Use the compiled shader program.
    glUseProgram(shader_program);



    /* Get all attribs and uniforms */
    GLint position_attrib = glGetAttribLocation(shader_program, "position");
    GLint datacoord_attrib = glGetAttribLocation(shader_program, "datacoord");
    GLint proj_uniform = glGetUniformLocation(shader_program, "proj");
    GLint view_uniform = glGetUniformLocation(shader_program, "view");
    GLint model_uniform = glGetUniformLocation(shader_program, "model");
    GLint datamap_uniform = glGetUniformLocation(shader_program, "datamap");
    GLint colormap_uniform = glGetUniformLocation(shader_program, "colormap");

    /* Upload vertices, face by face */
    GLuint vaos[12];
    glGenVertexArrays(12, vaos);
    static const unsigned int ndiv = 16;
    {
        int i, j;

        GLuint ebo;
        {
            GLushort elements[ndiv * ndiv * 3 * 2];
            GLushort *el = elements;
            for (i = 0; i < ndiv; i ++)
            {
                for (j = 0; j < ndiv; j ++)
                {
                    *el++ = (i+0) * (ndiv + 1) + (j+0);
                    *el++ = (i+0) * (ndiv + 1) + (j+1);
                    *el++ = (i+1) * (ndiv + 1) + (j+0);
                    *el++ = (i+1) * (ndiv + 1) + (j+0);
                    *el++ = (i+0) * (ndiv + 1) + (j+1);
                    *el++ = (i+1) * (ndiv + 1) + (j+1);
                }
            }

            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);
        }

        unsigned char ibase;
        for (ibase = 0; ibase < 12; ibase ++)
        {
            glBindVertexArray(vaos[ibase]);

            /* Upload vertex buffer */
            {
                GLfloat vertices[ndiv+1][ndiv+1][5];
                const float x0 = base_tile_xys[ibase][0];
                const float y0 = base_tile_xys[ibase][1];
                for (i = 0; i <= ndiv; i ++)
                {
                    for (j = 0; j <= ndiv; j ++)
                    {
                        float x, y, z, phi;
                        xy2zphi(x0 - 0.25 * (j - i) / ndiv, y0 + 0.25 * (i + j) / ndiv, &z, &phi);
                        x = cosf(phi) * sqrtf(1 - squaref(z));
                        y = sinf(phi) * sqrtf(1 - squaref(z));
                        float s = (float)i / ndiv;
                        float t = (float)j / ndiv;
                        vertices[i][j][0] = x;
                        vertices[i][j][1] = y;
                        vertices[i][j][2] = z;
                        vertices[i][j][3] = s;
                        vertices[i][j][4] = t;
                    }
                }

                GLuint vbo;
                glGenBuffers(1, &vbo);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            }

            /* We'll reuse the same ebo for each face. */
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

            /* Specify attribute layout */
            glEnableVertexAttribArray(position_attrib);
            glVertexAttribPointer(position_attrib, 3, GL_FLOAT, GL_FALSE,
                5 * sizeof(GLfloat), 0);

            glEnableVertexAttribArray(datacoord_attrib);
            glVertexAttribPointer(datacoord_attrib, 2, GL_FLOAT, GL_FALSE,
                5 * sizeof(GLfloat), (void *) (3 * sizeof(GLfloat)));
        }
    }

    /* Pass active textures */
    glUniform1i(datamap_uniform, 0);
    glUniform1i(colormap_uniform, 1);

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    /* Switch to active texture 0 so that we can cycle through tiles */
    glActiveTexture(GL_TEXTURE0);

    // Define keyboard callback. Escape on Esc key pressed.
    //glfwSetKeyCallback(window, keyboard);

    // Define mouse position callback. Rotate the sphere, when moving the mouse.
    glfwSetCursorPosCallback(window, motion);

    // Define mouse scroll callback. Zoom in and out, when using the scroll wheel.
    glfwSetScrollCallback(window, scroll);

    glClearColor(0, 0, 0, 0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    do  // Frame loop.
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        // Rotation model.
        mat4 model;
        model = rotate(model, 0.01f*mousex, vec3(0.0f, 0.0f, 1.0f));
        model = rotate(model, -0.01f*mousey, vec3(0.0f, 1.0f, 0.0f));

        mat4 proj = perspective(45.0f, (float)width/height, 1.0f, 10.0f);
        glViewport(0, 0, width, height);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, value_ptr(model));
        glUniformMatrix4fv(proj_uniform, 1, GL_FALSE, value_ptr(proj));
        mat4 view = lookAt(
            vec3(3.0f + 0.01f*scrolly, 0.0f, 0.0f),
            vec3(0.0f, 0.0f, 0.0f),
            vec3(0.0f, 0.0f, 1.0f)
        );
        glUniformMatrix4fv(view_uniform, 1, GL_FALSE, value_ptr(view));

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        unsigned char iface;
        for (iface = 0; iface < 12; iface ++)
        {
            glBindVertexArray(vaos[iface]);
            glBindTexture(GL_TEXTURE_2D, textures[iface]);
            glDrawElements(GL_TRIANGLES, ndiv*ndiv*3*2, GL_UNSIGNED_SHORT, 0);
        }
        glFlush();
        glfwSwapBuffers(window);
        glfwWaitEvents();

    } // Check if the ESC key was pressed or the window was closed
    while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0 );

    // Cleanup and exit.
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);

}
