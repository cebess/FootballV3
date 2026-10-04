#pragma once
enum TeamSide {
    HOME,
    VISITOR
};

// function to convert TeamSide enum to string for printing
String teamSideToString(TeamSide side);
TeamSide switchTeamSide(TeamSide side);