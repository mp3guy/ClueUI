/*
 * GameComponents.h
 *
 *  Created on: 9 Jan 2011
 *      Author: thomas
 */

#ifndef GAMECOMPONENTS_H_
#define GAMECOMPONENTS_H_

#include <string>

enum Status
{
    UNKNOWN,
    POSSESSED_BY,
    NOT_POSSESSED_BY,
    NUM_STATUSES
};

static const std::string statusSymbols[NUM_STATUSES] =
{
    " ",
    "X",
    "-"
};

#endif /* GAMECOMPONENTS_H_ */
