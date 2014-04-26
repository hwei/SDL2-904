//
//  resource.cpp
//  SDL2-904
//
//  Created by Huang Wei on 14-4-26.
//  Copyright (c) 2014年 hweigame. All rights reserved.
//

#include "resource.h"
#include <CoreFoundation/CFBundle.h>
#include <fstream>

namespace hardrock
{
    struct CFDel
    {
        void operator() (CFTypeRef ref) { CFRelease(ref); }
    };
    
    template <typename T>
    std::unique_ptr<T, CFDel> create_cfptr(T* ref)
    {
        return std::unique_ptr<T, CFDel>(ref);
    }
    
    int load_resource(const char* path, void* out_buffer, size_t out_size)
    {
        auto system_encoding = CFStringGetSystemEncoding();
        auto cp_path = create_cfptr(CFStringCreateWithCString(kCFAllocatorDefault, path, system_encoding));
        auto main_bundle = CFBundleGetMainBundle();
        auto cp_data_url = create_cfptr(
                                        CFBundleCopyResourceURL(main_bundle, cp_path.get(), nullptr, nullptr));
        auto cp_data_path = create_cfptr(CFURLCopyFileSystemPath(cp_data_url.get(), kCFURLPOSIXPathStyle));
        const char* data_path = CFStringGetCStringPtr(cp_data_path.get(), system_encoding);
        
        std::ifstream file(data_path, std::ios::binary);
        if (!file)
            return -1;
        file.seekg(0, file.end);
        size_t file_size = file.tellg();
        do
        {
            if (out_buffer == nullptr || out_size < file_size)
                break;
            file.seekg(0, file.beg);
            file.read(static_cast<char*>(out_buffer), file_size);
        } while (false);
        return static_cast<int>(file_size);
    }

}