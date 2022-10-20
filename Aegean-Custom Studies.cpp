#include "sierrachart.h"
#include <string>
#include <sstream>
#include <vector>
SCDLLName("Historical Levels")

struct PriceLabel {
    float Price;
    SCString Label;
    COLORREF LabelColor;
};

void DrawToChart(HWND WindowHandle, HDC DeviceContext, SCStudyInterfaceRef sc);

SCSFExport scsf_GoogleSheetsLevelsImporter(SCStudyInterfaceRef sc)
{
    // logging object
    SCString msg;

    SCInputRef i_FilePath = sc.Input[0];
    SCInputRef i_Transparency = sc.Input[1];

    // chart related settings
    SCInputRef i_DrawLinesOnChart = sc.Input[3];
    SCInputRef i_ShowPriceOnChart = sc.Input[4];

    // (separate) DOM related settings
    SCInputRef i_ShowLabelOnDom = sc.Input[5];
    SCInputRef i_DomFontSize = sc.Input[6];
    SCInputRef i_xOffset = sc.Input[7];
    SCInputRef i_yOffset = sc.Input[8];
    SCInputRef i_LabelBgColor = sc.Input[9];

    // Set configuration variables
    if (sc.SetDefaults)
    {
        sc.GraphName = "Historical Levels Importer";
        sc.GraphRegion = 0;

        i_FilePath.Name = "Google Sheets URL";
        i_FilePath.SetString("https://docs.google.com/spreadsheets/d/1WkN119o3WY8HFWi6EM7ANJxqx8xuW2xSedC_busu0LE");

        i_Transparency.Name = "Transparency Level";
        i_Transparency.SetInt(70);

        // Chart specific settings
        i_DrawLinesOnChart.Name = "Draw Lines On Chart?";
        i_DrawLinesOnChart.SetYesNo(1);

        i_ShowPriceOnChart.Name = "> Show Price By Chart Label?";
        i_ShowPriceOnChart.SetYesNo(0);


        // DOM specific settings
        i_ShowLabelOnDom.Name = "Show Label On DOM?";
        i_ShowLabelOnDom.SetYesNo(0);

        i_DomFontSize.Name = " > DOM Font Size";
        i_DomFontSize.SetInt(18);

        i_xOffset.Name = " > DOM Label X-coord Offset";
        i_xOffset.SetInt(-70);

        i_yOffset.Name = " > DOM Label Y-coord Offset";
        i_yOffset.SetInt(-10);

        i_LabelBgColor.Name = " > DOM Label Background Color";
        i_LabelBgColor.SetColor(255,255,255);

        return;
    }


    SCString &HttpResponseContent = sc.GetPersistentSCString(4);
    SCString Url = i_FilePath.GetString();
    Url.Format("%s/gviz/tq?tqx=out:csv", Url.GetChars());

    std::vector<PriceLabel>* p_PriceLabels = reinterpret_cast<std::vector<PriceLabel>*>(sc.GetPersistentPointer(0));
    if (p_PriceLabels == NULL) {
        p_PriceLabels = new std::vector<PriceLabel>;
        sc.SetPersistentPointer(0, p_PriceLabels);
    }
    else {
        p_PriceLabels->clear();
    }

    enum {REQUEST_NOT_SENT = 0,  REQUEST_SENT, REQUEST_RECEIVED};
    // latest request status
    int& RequestState = sc.GetPersistentInt(1);

    if (sc.Index == 0) {
        if (RequestState == REQUEST_NOT_SENT)
        {
            if (!sc.MakeHTTPRequest(Url))
            {
                sc.AddMessageToLog("Error making HTTP request.", 1);
                RequestState = REQUEST_NOT_SENT;
            }
            else
                RequestState = REQUEST_SENT;
        }

    }

    if (RequestState == REQUEST_SENT && sc.HTTPResponse != "")
    {
        RequestState = REQUEST_RECEIVED;
        HttpResponseContent = sc.HTTPResponse;
    }
    else if (RequestState == REQUEST_SENT && sc.HTTPResponse == "")
    {
        return;
    }

    RequestState = REQUEST_NOT_SENT;

    std::vector<char*> tokens;
    std::istringstream input(HttpResponseContent.GetChars());
    int LineNumber = 1;
    for (std::string line; getline(input,line);) {
        msg.Format("%s", line.c_str());

        SCString scline = line.c_str();
        scline = scline.GetSubString(scline.GetLength() - 2, 1);
        scline.Tokenize("\",\"", tokens);

        s_UseTool Tool;
        Tool.LineStyle = LINESTYLE_SOLID;
        Tool.LineWidth = 1;
        Tool.TextAlignment = DT_RIGHT;
        int idx = 1;
        // used for lines and rectangles
        float price;
        // only used for rectangles
        float price2 = 0;
        SCString note;
        SCString color;
        int linewidth = 1;
        int textalignment = 1;
		int year;
        int month;
        int day;
        SCDateTime SCDrawDate;
        int beginDrawDateIndex = -1; // set to -1 to be able to check whether the value has changed later on
        int endDrawDateIndex = -1;
        for (char* i : tokens) {
            //msg.Format("%s", i);
            //sc.AddMessageToLog(msg,1);

            // price
            if (idx == 1) {
                price = atof(i);
            }
            // price 2 (only used for rectangles)
            else if (idx == 2) {
                price2 = atof(i);
                if (price2 == 0) {
                    Tool.DrawingType = DRAWING_LINE;
                    Tool.BeginValue = price;
                    Tool.EndValue = price;
                }
                else {
                    Tool.DrawingType = DRAWING_RECTANGLE_EXT_HIGHLIGHT;
                    Tool.BeginValue = price;
                    Tool.EndValue = price2;
                }
            }
            // note label
            else if (idx == 3) {
                note = i;
            }
            // color
            else if (idx == 4) {
                color = i;

                // could use a switch here, but most will find this easier to read:
                if (color == "red") Tool.Color = COLOR_RED;
                else if (color == "green") Tool.Color = COLOR_GREEN;
                else if (color == "blue") Tool.Color = COLOR_BLUE;
                else if (color == "white") Tool.Color = COLOR_WHITE;
                else if (color == "black") Tool.Color = COLOR_BLACK;
                else if (color == "purple") Tool.Color = COLOR_PURPLE;
                else if (color == "pink") Tool.Color = COLOR_PINK;
                else if (color == "yellow") Tool.Color = COLOR_YELLOW;
                else if (color == "gold") Tool.Color = COLOR_GOLD;
                else if (color == "brown") Tool.Color = COLOR_BROWN;
                else if (color == "cyan") Tool.Color = COLOR_CYAN;
                else if (color == "gray") Tool.Color = COLOR_GRAY;
                else Tool.Color = COLOR_WHITE;

                // if drawing a rectangle, make the fill color same as border
                if (price2 > 0) Tool.SecondaryColor = Tool.Color;
            }
            // line type
            else if (idx == 5) {
                int LineType = atoi(i);
                if (LineType == 0) Tool.LineStyle = LINESTYLE_SOLID;
                else if (LineType == 1) Tool.LineStyle = LINESTYLE_DASH;
                else if (LineType == 2) Tool.LineStyle = LINESTYLE_DOT;
                else if (LineType == 3) Tool.LineStyle = LINESTYLE_DASHDOT;
                else if (LineType == 4) Tool.LineStyle = LINESTYLE_DASHDOTDOT;
            }
            // line width
            else if (idx == 6) {
                linewidth = atoi(i);
                if (linewidth > 0) Tool.LineWidth = linewidth;
            }
            // text alignment
            else if (idx == 7) {
                textalignment = atoi(i);
                if (textalignment > 0) Tool.TextAlignment = textalignment;
            }

            // Year
            else if (idx == 8)
            {
                year = atoi(i);
            }

            // Month
            else if (idx == 9)
            {
                month = atoi(i);
            }

            // Day
            else if (idx == 10)
            {
                day = atoi(i);

                for (int j = 0; j < sc.BaseDateTimeIn[sc.ArraySize]; j++)
                {
                    int idxYear = 0;
                    int idxMonth = 0;
                    int idxDay = 0;
                    sc.BaseDateTimeIn[j].GetDateYMD(idxYear, idxMonth, idxDay);

                    if (idxYear == year 
                        && idxMonth == month 
                        && idxDay == day 
                        && beginDrawDateIndex == -1)
                    {
                        // set beginDrawDateIndex at first matching date (assumes data is sorted)
                        beginDrawDateIndex = j;
                    }
                    
                    else if (idxYear == year 
                        && idxMonth == month 
                        && idxDay == day)
                    {
                        // continue setting the endDrawDateIndex until the dates stop matching (assumes data is sorted)
                        endDrawDateIndex = j;
                    }
                }

                if (endDrawDateIndex != -1 && beginDrawDateIndex != -1)
                {
                    //sc.AddMessageToLog("Date index variables set!", 1);
                }
            }

            // draw line
            if (i_DrawLinesOnChart.GetInt() == 1) {
				Tool.ChartNumber = sc.ChartNumber;
				if (beginDrawDateIndex == -1)
				{
					Tool.BeginDateTime = sc.BaseDateTimeIn[0];
					Tool.EndDateTime = sc.BaseDateTimeIn[sc.ArraySize-1];
				}
				else
				{
					Tool.BeginDateTime = sc.BaseDateTimeIn[beginDrawDateIndex];
					Tool.EndDateTime = sc.BaseDateTimeIn[endDrawDateIndex];
				}
                Tool.AddMethod = UTAM_ADD_OR_ADJUST;
                Tool.ShowPrice = i_ShowPriceOnChart.GetInt();
                Tool.TransparencyLevel = i_Transparency.GetInt();
                Tool.Text = note;
                Tool.LineNumber = LineNumber;
                sc.UseTool(Tool);
            }

            // increment field counter
            idx++;
        }

        // increment row counter
        LineNumber++;

        // if we want to show on DOM, we need to add to our list
        if (i_ShowLabelOnDom.GetInt() == 1 && price > 0 && note != "") {
            PriceLabel TmpPriceLabel = { price, note, Tool.Color };
            if (price2 > 0) {
                TmpPriceLabel.Label.Format("Rect1:%s", note.GetChars());
            }
            //PriceLabels.insert(PriceLabels.end(), TmpPriceLabel);
            p_PriceLabels->insert(p_PriceLabels->end(), TmpPriceLabel);
//msg.Format("Adding %f => %s", price, note.GetChars());
//sc.AddMessageToLog(msg, 1);

            if (price2 > 0) {
                TmpPriceLabel = { price2, note, Tool.Color };
                TmpPriceLabel.Label.Format("Rect2:%s", note.GetChars());
                p_PriceLabels->insert(p_PriceLabels->end(), TmpPriceLabel);
            }
        }
    }

    sc.p_GDIFunction = DrawToChart;
}

void DrawToChart(HWND WindowHandle, HDC DeviceContext, SCStudyInterfaceRef sc)
{

    SCString log;
//    // keep track of last write time
//    SCDateTime CurrTime = sc.CurrentSystemDateTime;
//    int UpdateIntervalSeconds = 10;
//    int &LastWrittenSec = sc.GetPersistentInt(5);
//log.Format("LastWrittenSec=%d vs %d, return=%d", LastWrittenSec, CurrTime.GetTimeInSeconds(), (LastWrittenSec + UpdateIntervalSeconds > CurrTime.GetTimeInSeconds()));
//sc.AddMessageToLog(log, 1);
//    if (LastWrittenSec + UpdateIntervalSeconds > CurrTime.GetTimeInSeconds()) {
//log.Format("Should be returning here...");
//sc.AddMessageToLog(log, 1);
//        return;
//    }
//    LastWrittenSec = CurrTime.GetTimeInSeconds();

    int xOffset = sc.Input[7].GetInt();
    int yOffset = sc.Input[8].GetInt();
    int x = sc.GetDOMColumnLeftCoordinate(n_ACSIL::DOM_COLUMN_GENERAL_PURPOSE_1);
    x += xOffset;
    int y;
    SCString msg;
    std::vector<PriceLabel> *p_PriceLabels = (std::vector<PriceLabel>*)sc.GetPersistentPointer(0);

    float plotPrice = 0;

    // grab the name of the font used in this chartbook
    int fontSize = sc.Input[6].GetInt();
    //int fontSize = 14;
    SCString chartFont = sc.ChartTextFont();

    // Windows GDI font creation
    // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createfonta
    HFONT hFont;
    hFont = CreateFont(fontSize,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
            CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY, DEFAULT_PITCH,TEXT(chartFont));

    // https://docs.microsoft.com/en-us/windows/win32/gdi/colorref
    //const COLORREF bg = 0x00C0C0C0;
    const COLORREF fg = 0x00000000;
    COLORREF bg = sc.Input[9].GetColor();
    // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-settextcolor
    ::SetTextColor(DeviceContext, fg);
    ::SetBkColor(DeviceContext, bg);

    // Windows GDI transparency 
    // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setbkmode
    //SetBkMode(DeviceContext, OPAQUE);
    SelectObject(DeviceContext, hFont);

    // Windows GDI transparency 
    // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setbkmode
    //SetBkMode(DeviceContext, TRANSPARENT);
    //int NumberOfDecimals = sc.Input[1].GetInt();
    int NumPriceLabels = p_PriceLabels->size();
//log.Format("%d PriceLabels found", NumPriceLabels);
//sc.AddMessageToLog(log,1);
    for (int i=0; i<NumPriceLabels; i++) {

        // fetch the PriceLabel we want to draw
        PriceLabel TmpPriceLabel = p_PriceLabels->at(i);
        plotPrice = TmpPriceLabel.Price;
        msg = TmpPriceLabel.Label;
        COLORREF LabelColor = (COLORREF)TmpPriceLabel.LabelColor;
        ::SetTextColor(DeviceContext, LabelColor);

        // calculate coords
        y = sc.RegionValueToYPixelCoordinate(plotPrice, sc.GraphRegion);
        y += yOffset;
        ::SetTextAlign(DeviceContext, TA_NOUPDATECP);
//log.Format("[%d/%d] %d (%f) => %s", (i+1), NumPriceLabels, y, plotPrice, msg.GetChars());
//sc.AddMessageToLog(log,1);
        ::TextOut(DeviceContext, x, y, msg, msg.GetLength());

        // delete font
        DeleteObject(hFont);
    }

    return;
}
