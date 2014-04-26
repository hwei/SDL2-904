//
//  glhandle.h
//  SDL2-904
//
//  Created by Huang Wei on 14-4-26.
//  Copyright (c) 2014年 hweigame. All rights reserved.
//

#ifndef SDL2_904_glhandle_h
#define SDL2_904_glhandle_h

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
#include <vector>

namespace hardrock
{
    template<typename OP>
    class GlHandles
    {
        std::vector<GLuint> handles;
    public:
        GlHandles(int n)
        : handles(n)
        {
            OP op;
            op.gen(n, &this->handles[0]);
        }
        ~GlHandles()
        {
            OP op;
            op.del((GLsizei)this->handles.size(), &this->handles[0]);
        }
        GLuint get(int i) const { return this->handles[i]; }
    };
    
    template<typename OP>
    class GlHandle
    {
        GLuint handle;
    public:
        template<typename ...Ts>
        GlHandle(Ts... args)
        {
            OP op;
            this->handle = op.gen(args...);
        }
        ~GlHandle()
        {
            OP op;
            op.del(this->handle);
        }
        GLuint get() const { return this->handle; }
        operator GLuint() const { return this->handle; }
    };
    
    struct OpGlBuffers
    {
        void gen(GLsizei n, GLuint *p) { glGenBuffers(n, p); }
        void del(GLsizei n, GLuint *p) { glDeleteBuffers(n, p); }
    };
    
    struct OpGlVertexArrays
    {
        void gen(GLsizei n, GLuint *p) { glGenVertexArrays(n, p); }
        void del(GLsizei n, GLuint *p) { glDeleteVertexArrays(n, p); }
    };
    
    struct OpGlTextures
    {
        void gen(GLsizei n, GLuint *p) { glGenTextures(n, p); }
        void del(GLsizei n, GLuint *p) { glDeleteTextures(n, p); }
    };
    
    struct OpGlShader
    {
        GLuint gen(GLenum type) { return glCreateShader(type); }
        void del(GLuint h) { glDeleteShader(h); }
    };
    
    struct OpGlProgram
    {
        GLuint gen() { return glCreateProgram(); }
        void del(GLuint h) { glDeleteShader(h); }
    };
}


#endif
