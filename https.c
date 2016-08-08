/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2015, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
/* <DESC>
 * Simple HTTPS GET
 * </DESC>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL) {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

int main(void)
{
    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if (curl) {
        struct MemoryStruct chunk;
        chunk.memory = (char *)malloc(1); /* will be grown as needed by the realloc above */
        chunk.size = 0;                   /* no data at this point */

        curl_easy_setopt(curl, CURLOPT_URL,
                         "https://us.api.battle.net/wow/auction/data/"
                         "Nemesis?locale=en_US&apikey=35cvxa396f8r9yah8pqqgsfzxyvjxby7");

        /* send all data to this function  */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

        /* we pass our 'chunk' struct to the callback function */
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        /* always cleanup */
        curl_easy_cleanup(curl);

        std::stringstream ss;
        ss << chunk.memory;

        // Create a root
        boost::property_tree::ptree get_json;

        // Load the json file in this ptree
        boost::property_tree::read_json(ss, get_json);

        std::string json_url;

        BOOST_FOREACH (const boost::property_tree::ptree::value_type &child,
                       get_json.get_child("files")) {
            std::cout << "url: " << child.second.get<std::string>("url") << "\n";
            std::cout << "lastModified:: " << child.second.get<std::string>("lastModified") << "\n";

            json_url = child.second.get<std::string>("url");
        }

        auto curl2 = curl_easy_init();
        if (curl2) {
            struct MemoryStruct chunk2;
            chunk2.memory = (char *)malloc(1); /* will be grown as needed by the realloc above */
            chunk2.size = 0;                   /* no data at this point */
            curl_easy_setopt(curl2, CURLOPT_URL, json_url.c_str());

            /* send all data to this function  */
            curl_easy_setopt(curl2, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

            /* we pass our 'chunk' struct to the callback function */
            curl_easy_setopt(curl2, CURLOPT_WRITEDATA, (void *)&chunk2);

            /* Perform the request, res will get the return code */
            res = curl_easy_perform(curl2);
            /* Check for errors */
            if (res != CURLE_OK) {
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            }

            /* always cleanup */
            curl_easy_cleanup(curl2);

            std::stringstream ss2;
            ss2 << chunk2.memory;

            // Create a root
            boost::property_tree::ptree get_json2;

            // Load the json file in this ptree
            boost::property_tree::read_json(ss2, get_json2);

            BOOST_FOREACH (const boost::property_tree::ptree::value_type &child,
                           get_json2.get_child("auctions")) {
                std::cout << " item: " << child.second.get<std::string>("item");
                std::cout << " buyout: " << child.second.get<std::string>("buyout");
                std::cout << " bid: " << child.second.get<std::string>("bid") << "\n";
            }
        }
    }
    curl_global_cleanup();

    return 0;
}
