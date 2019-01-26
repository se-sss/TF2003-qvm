/*
 *  QWProgs-TF2003
 *  Copyright (C) 2004  [sd] angel
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *  $Id: vote.c,v 1.8 2006-11-21 16:41:57 AngelD Exp $
 */
   
// vote.q: mapchange voting functions
#include "g_local.h"

int Vote_ChangeMap_Init();
void Vote_ChangeMap_Run();

int Vote_Admin_Init();
void Vote_Admin_Run();
static float elect_percentage;
static int elect_level;
static gedict_t* elect_player;


int CheckString(char *str);
const vote_t votes[]=
{
	{"changemap", Vote_ChangeMap_Init, Vote_ChangeMap_Run, 60, 3},
	{"admin", Vote_Admin_Init, Vote_Admin_Run, 60, 3},
//	{"map", Vote_Map_Init, Vote_Map_Yes, Vote_Map_No, 60, 3},
	{NULL}
};
static int		k_vote = 0;
int 		current_vote = -1;


void NextLevel();


int CountPlayers()
{
	gedict_t	*p=world;
	int 		num = 0;

	while((p = trap_find(p, FOFS(s.v.classname), "player"))) 
		if(p->s.v.netname[0]) 
			num++;

	return num;
}

void _clearVote()
{
	gedict_t	*p = world;
    k_vote = 0;
    current_vote = -1;
    while((p = trap_find(p, FOFS(s.v.classname), "player"))) 
    {
        p->k_voted = 0;
    }

    p = trap_find(world, FOFS(s.v.classname), "voteguard");
    if(p) {
        p->s.v.classname  = "";
        dremove(p);
    }
}

int _checkVote()
{
	int f1, needed_votes;

	f1 = CountPlayers();
	needed_votes = (int)(f1 * elect_percentage / 100)+1 - k_vote;

	if( needed_votes < 1)
        return 0;

	if( needed_votes > 1)
		G_bprint(2, "�%d� more votes needed\n",f1);
	else
		G_bprint(2, "�%d� more vote needed\n",f1);	
    return needed_votes;
}

int _addVote()
{
	self->k_voted = 1;
    if( k_vote > 0 )
        G_bprint(2, "%s gives his vote\n",self->s.v.netname);
    k_vote++;

	if( _checkVote() < 1)
	{
        _clearVote();
		return 1;	
	}

    return 0;
}

void _subVote()
{
	int f1, needed_votes;

	if(!k_vote || !self->k_voted) return;
// withdraw one's vote
    G_bprint(2, "%s withdraws his vote\n",self->s.v.netname);
	self->k_voted = 0;
	k_vote--;
	if(k_vote < 1) {
		G_bprint(3, "Voting is closed\n");
        _clearVote();
		return;
	}
    _checkVote();
}

void VoteThink()
{
    gedict_t* p=world;

    G_bprint(2, "The voting has timed out.\n");
    self->s.v.nextthink = -1;
    k_vote = 0;
    current_vote = -1;
    while((p = trap_find(p, FOFS(s.v.classname), "player"))) 
    {
        if(p->s.v.netname[0] ) p->k_voted = 0;
    }
    dremove(self);
}

void _startVote()
{
	gedict_t* voteguard;

	voteguard = spawn(); // Check the 1 minute timeout for voting
	voteguard->s.v.owner = EDICT_TO_PROG(world);
	voteguard->s.v.classname  = "voteguard";
	voteguard->s.v.think = (func_t) VoteThink;
	voteguard->s.v.nextthink = g_globalvars.time + votes[current_vote].timeout;
}

int Vote_ChangeMap_Init()
{
	G_bprint(2, "%s votes for mapchange\n",self->s.v.netname);
    return 1;
}

void Vote_ChangeMap_Run()
{
    G_bprint(3, "Map changed by majority vote\n");
    NextLevel();
}

int Vote_Admin_Init()
{
	elect_percentage = GetSVInfokeyInt(  "electpercentage", NULL, 50 );
	elect_level  = GetSVInfokeyInt(  "electlevel", NULL, 1 );
	elect_player = self;
	if(!elect_percentage || !elect_level)
	{
		G_sprint( self, 2, "Admin election disabled\n",self->s.v.netname);
		return 0;
	}

	if ( self->is_admin >= elect_level ) 
	{
		G_sprint(self, 2, "You are already an admin\n");
		return 0;
	}

    return 1;
}

void Vote_Admin_Run()
{
	int 		f1, needed_votes;

    G_bprint(2,  "%s ����� ����� ������!\n", elect_player->s.v.netname);
    G_sprint(elect_player, 2, "Type ��� ����� for admin commands.\n");
	elect_level  = GetSVInfokeyInt(  "electlevel", NULL, 1 );
    elect_player->is_admin = elect_level;
}

void Vote_Cmd()
{
    char    cmd_command[50];
    int     argc,i;
    const vote_t*ucmd;

    if ( tf_data.cb_prematch_time > g_globalvars.time )
    {
        return;
    }

    argc = trap_CmdArgc();

    if( argc != 2 )
    {
        G_sprint( self, 2, "Avaliable votes:\n");
        for ( ucmd = votes ; ucmd->command  ; ucmd++ )
        {
            G_sprint( self, 2, "%s\n",ucmd->command);
        }
        return;
    }

    trap_CmdArgv( 1, cmd_command, sizeof( cmd_command ) );

    elect_percentage = GetSVInfokeyInt(  "electpercentage", NULL, 50 );
	if(!elect_percentage)
	{
		G_sprint( self, 2, "Votes disabled\n");
		return;
	}
	if( elect_percentage < 5 || elect_percentage > 95)
		elect_percentage = 50;

    if( current_vote != -1 )
    {
        if(!strcmp(cmd_command,"yes"))
        {
            if(self->k_voted) {
                G_sprint(self, 2, "--- your vote is still good ---\n");
                return;
            }
            i = current_vote;
            if( _addVote() )
                votes[i].VoteRun();
            return;
        }

        if(!strcmp(cmd_command,"no"))
        {
            if(self->k_voted) {
                _subVote();
            }
            G_sprint(self, 2, "--- your vote: no ---\n");
            //votes[current_vote].VoteNo();
            return;
        }
        G_sprint( self, 2, "Vote %s in progress\n",votes[current_vote].command);
        return;
    }
    if( g_globalvars.time < self->last_vote_time )
    {
        G_sprint( self, 2, "You cannot vote at this time.\n");
        return;
    }

    for ( ucmd = votes,i=0 ; ucmd->command  ; ucmd++,i++ )
    {
        if( !strcmp(cmd_command,ucmd->command) )
        {
            current_vote = i;
            k_vote = 0;
            self->last_vote_time = g_globalvars.time + votes[current_vote].pause * 60;
            if( ucmd->VoteInit())
            {
                if ( _addVote() )
                {
                    votes[i].VoteRun();
                    return;
                }
                G_bprint(3, "��� ���� ���");
                G_bprint(2, " in console to approve\n");
                _startVote();
            }
            return;
        }
    }
    G_sprint( self, 2, "Unknown vote.\n");
}
