VERSION 5.00
Begin VB.Form Form1 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Euro1943 Map Editor"
   ClientHeight    =   5670
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   9690
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   5670
   ScaleWidth      =   9690
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   46
      Left            =   3120
      TabIndex        =   58
      Top             =   5280
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   45
      Left            =   720
      TabIndex        =   57
      Top             =   5280
      Width           =   975
   End
   Begin VB.CommandButton cmdClear 
      Caption         =   "Clear to Selected"
      Height          =   495
      Left            =   5160
      TabIndex        =   56
      Top             =   2760
      Width           =   1095
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   44
      Left            =   8640
      TabIndex        =   55
      Top             =   5280
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   43
      Left            =   7560
      TabIndex        =   54
      Top             =   5280
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   42
      Left            =   6480
      TabIndex        =   53
      Top             =   5280
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   41
      Left            =   8640
      TabIndex        =   52
      Top             =   4920
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   40
      Left            =   7560
      TabIndex        =   51
      Top             =   4920
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   39
      Left            =   6480
      TabIndex        =   50
      Top             =   4920
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   38
      Left            =   8640
      TabIndex        =   49
      Top             =   4560
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   37
      Left            =   7560
      TabIndex        =   48
      Top             =   4560
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   36
      Left            =   6480
      TabIndex        =   47
      Top             =   4560
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   35
      Left            =   8640
      TabIndex        =   46
      Top             =   4200
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   34
      Left            =   7560
      TabIndex        =   45
      Top             =   4200
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   33
      Left            =   6480
      TabIndex        =   44
      Top             =   4200
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   32
      Left            =   8640
      TabIndex        =   43
      Top             =   3840
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   31
      Left            =   7560
      TabIndex        =   42
      Top             =   3840
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   30
      Left            =   6480
      TabIndex        =   41
      Top             =   3840
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   29
      Left            =   8640
      TabIndex        =   37
      Top             =   3480
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   28
      Left            =   7560
      TabIndex        =   36
      Top             =   3480
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   27
      Left            =   6480
      TabIndex        =   35
      Top             =   3480
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   26
      Left            =   8640
      TabIndex        =   34
      Top             =   3120
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   25
      Left            =   7560
      TabIndex        =   33
      Top             =   3120
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   24
      Left            =   6480
      TabIndex        =   32
      Top             =   3120
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   23
      Left            =   8640
      TabIndex        =   31
      Top             =   2760
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   22
      Left            =   7560
      TabIndex        =   30
      Top             =   2760
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   21
      Left            =   6480
      TabIndex        =   29
      Top             =   2760
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   20
      Left            =   8640
      TabIndex        =   28
      Top             =   2400
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   19
      Left            =   7560
      TabIndex        =   27
      Top             =   2400
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   18
      Left            =   6480
      TabIndex        =   26
      Top             =   2400
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   17
      Left            =   8640
      TabIndex        =   25
      Top             =   2040
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   16
      Left            =   7560
      TabIndex        =   24
      Top             =   2040
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   15
      Left            =   6480
      TabIndex        =   23
      Top             =   2040
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   14
      Left            =   8640
      TabIndex        =   22
      Top             =   1680
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   13
      Left            =   7560
      TabIndex        =   21
      Top             =   1680
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Enabled         =   0   'False
      Height          =   285
      Index           =   12
      Left            =   6480
      TabIndex        =   20
      Text            =   "2"
      Top             =   1680
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   11
      Left            =   8640
      TabIndex        =   19
      Top             =   1320
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   10
      Left            =   7560
      TabIndex        =   18
      Top             =   1320
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Enabled         =   0   'False
      Height          =   285
      Index           =   9
      Left            =   6480
      TabIndex        =   17
      Text            =   "1"
      Top             =   1320
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   8
      Left            =   8640
      TabIndex        =   16
      Top             =   960
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   7
      Left            =   7560
      TabIndex        =   15
      Top             =   960
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Enabled         =   0   'False
      Height          =   285
      Index           =   6
      Left            =   6480
      TabIndex        =   14
      Text            =   "0"
      Top             =   960
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   5
      Left            =   8640
      TabIndex        =   13
      Top             =   600
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   4
      Left            =   7560
      TabIndex        =   12
      Top             =   600
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Enabled         =   0   'False
      Height          =   285
      Index           =   3
      Left            =   6480
      TabIndex        =   11
      Text            =   "0"
      Top             =   600
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   2
      Left            =   8640
      TabIndex        =   10
      Top             =   240
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Height          =   285
      Index           =   1
      Left            =   7560
      TabIndex        =   9
      Top             =   240
      Width           =   975
   End
   Begin VB.TextBox txtObjs 
      Enabled         =   0   'False
      Height          =   285
      Index           =   0
      Left            =   6480
      TabIndex        =   8
      Text            =   "0"
      Top             =   240
      Width           =   975
   End
   Begin VB.CommandButton cmdNew 
      Caption         =   "New"
      Height          =   255
      Left            =   5160
      TabIndex        =   7
      Top             =   3360
      Width           =   1095
   End
   Begin VB.CommandButton cmdRef 
      Caption         =   "Refresh"
      Height          =   255
      Left            =   5160
      TabIndex        =   6
      Top             =   2520
      Width           =   1095
   End
   Begin VB.CommandButton cmdLoad 
      Caption         =   "Load"
      Height          =   375
      Left            =   5160
      TabIndex        =   3
      Top             =   4560
      Width           =   1095
   End
   Begin VB.CommandButton cmdSave 
      Caption         =   "Save"
      Height          =   375
      Left            =   5160
      TabIndex        =   2
      Top             =   4200
      Width           =   1095
   End
   Begin VB.VScrollBar vsb 
      Height          =   4815
      LargeChange     =   10
      Left            =   4800
      Max             =   90
      TabIndex        =   1
      Top             =   0
      Width           =   255
   End
   Begin VB.HScrollBar hsb 
      Height          =   255
      LargeChange     =   10
      Left            =   0
      Max             =   90
      TabIndex        =   0
      Top             =   4800
      Width           =   4815
   End
   Begin VB.Label Label3 
      Caption         =   "y"
      Height          =   255
      Left            =   8880
      TabIndex        =   40
      Top             =   0
      Width           =   495
   End
   Begin VB.Label Label2 
      Caption         =   "x"
      Height          =   255
      Left            =   7920
      TabIndex        =   39
      Top             =   0
      Width           =   615
   End
   Begin VB.Label Label1 
      Caption         =   "type"
      Height          =   255
      Left            =   6600
      TabIndex        =   38
      Top             =   0
      Width           =   735
   End
   Begin VB.Label lblY 
      Caption         =   "0"
      Height          =   255
      Left            =   5160
      TabIndex        =   5
      Top             =   2160
      Width           =   255
   End
   Begin VB.Label lblX 
      Caption         =   "0"
      Height          =   255
      Left            =   2280
      TabIndex        =   4
      Top             =   5040
      Width           =   375
   End
   Begin VB.Shape shpSel 
      BorderWidth     =   2
      Height          =   495
      Left            =   5520
      Top             =   0
      Width           =   495
   End
   Begin VB.Image pal 
      Height          =   480
      Index           =   4
      Left            =   5520
      Picture         =   "Form1.frx":0000
      Top             =   1920
      Width           =   480
   End
   Begin VB.Image pal 
      Height          =   480
      Index           =   3
      Left            =   5520
      Picture         =   "Form1.frx":0C42
      Top             =   1440
      Width           =   480
   End
   Begin VB.Image pal 
      Height          =   480
      Index           =   2
      Left            =   5520
      Picture         =   "Form1.frx":1884
      Top             =   960
      Width           =   480
   End
   Begin VB.Image pal 
      Height          =   480
      Index           =   1
      Left            =   5520
      Picture         =   "Form1.frx":24C6
      Top             =   480
      Width           =   480
   End
   Begin VB.Image pal 
      Height          =   480
      Index           =   0
      Left            =   5520
      Picture         =   "Form1.frx":3108
      Top             =   0
      Width           =   480
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   99
      Left            =   4320
      Top             =   4320
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   98
      Left            =   3840
      Top             =   4320
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   97
      Left            =   3360
      Top             =   4320
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   96
      Left            =   2880
      Top             =   4320
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   95
      Left            =   2400
      Top             =   4320
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   94
      Left            =   1920
      Top             =   4320
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   93
      Left            =   1440
      Top             =   4320
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   92
      Left            =   960
      Top             =   4320
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   91
      Left            =   480
      Top             =   4320
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   90
      Left            =   0
      Top             =   4320
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   89
      Left            =   4320
      Top             =   3840
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   88
      Left            =   3840
      Top             =   3840
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   87
      Left            =   3360
      Top             =   3840
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   86
      Left            =   2880
      Top             =   3840
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   85
      Left            =   2400
      Top             =   3840
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   84
      Left            =   1920
      Top             =   3840
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   83
      Left            =   1440
      Top             =   3840
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   82
      Left            =   960
      Top             =   3840
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   81
      Left            =   480
      Top             =   3840
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   80
      Left            =   0
      Top             =   3840
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   79
      Left            =   4320
      Top             =   3360
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   78
      Left            =   3840
      Top             =   3360
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   77
      Left            =   3360
      Top             =   3360
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   76
      Left            =   2880
      Top             =   3360
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   75
      Left            =   2400
      Top             =   3360
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   74
      Left            =   1920
      Top             =   3360
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   73
      Left            =   1440
      Top             =   3360
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   72
      Left            =   960
      Top             =   3360
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   71
      Left            =   480
      Top             =   3360
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   70
      Left            =   0
      Top             =   3360
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   69
      Left            =   4320
      Top             =   2880
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   68
      Left            =   3840
      Top             =   2880
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   67
      Left            =   3360
      Top             =   2880
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   66
      Left            =   2880
      Top             =   2880
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   65
      Left            =   2400
      Top             =   2880
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   64
      Left            =   1920
      Top             =   2880
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   63
      Left            =   1440
      Top             =   2880
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   62
      Left            =   960
      Top             =   2880
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   61
      Left            =   480
      Top             =   2880
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   60
      Left            =   0
      Top             =   2880
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   59
      Left            =   4320
      Top             =   2400
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   58
      Left            =   3840
      Top             =   2400
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   57
      Left            =   3360
      Top             =   2400
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   56
      Left            =   2880
      Top             =   2400
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   55
      Left            =   2400
      Top             =   2400
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   54
      Left            =   1920
      Top             =   2400
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   53
      Left            =   1440
      Top             =   2400
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   52
      Left            =   960
      Top             =   2400
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   51
      Left            =   480
      Top             =   2400
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   50
      Left            =   0
      Top             =   2400
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   49
      Left            =   4320
      Top             =   1920
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   48
      Left            =   3840
      Top             =   1920
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   47
      Left            =   3360
      Top             =   1920
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   46
      Left            =   2880
      Top             =   1920
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   45
      Left            =   2400
      Top             =   1920
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   44
      Left            =   1920
      Top             =   1920
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   43
      Left            =   1440
      Top             =   1920
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   42
      Left            =   960
      Top             =   1920
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   41
      Left            =   480
      Top             =   1920
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   40
      Left            =   0
      Top             =   1920
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   39
      Left            =   4320
      Top             =   1440
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   38
      Left            =   3840
      Top             =   1440
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   37
      Left            =   3360
      Top             =   1440
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   36
      Left            =   2880
      Top             =   1440
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   35
      Left            =   2400
      Top             =   1440
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   34
      Left            =   1920
      Top             =   1440
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   33
      Left            =   1440
      Top             =   1440
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   32
      Left            =   960
      Top             =   1440
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   31
      Left            =   480
      Top             =   1440
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   30
      Left            =   0
      Top             =   1440
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   29
      Left            =   4320
      Top             =   960
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   28
      Left            =   3840
      Top             =   960
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   27
      Left            =   3360
      Top             =   960
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   26
      Left            =   2880
      Top             =   960
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   25
      Left            =   2400
      Top             =   960
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   24
      Left            =   1920
      Top             =   960
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   23
      Left            =   1440
      Top             =   960
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   22
      Left            =   960
      Top             =   960
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   21
      Left            =   480
      Top             =   960
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   20
      Left            =   0
      Top             =   960
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   19
      Left            =   4320
      Top             =   480
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   18
      Left            =   3840
      Top             =   480
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   17
      Left            =   3360
      Top             =   480
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   16
      Left            =   2880
      Top             =   480
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   15
      Left            =   2400
      Top             =   480
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   14
      Left            =   1920
      Top             =   480
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   13
      Left            =   1440
      Top             =   480
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   12
      Left            =   960
      Top             =   480
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   11
      Left            =   480
      Top             =   480
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   10
      Left            =   0
      Top             =   480
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   9
      Left            =   4320
      Top             =   0
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   8
      Left            =   3840
      Top             =   0
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   7
      Left            =   3360
      Top             =   0
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   6
      Left            =   2880
      Top             =   0
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   5
      Left            =   2400
      Top             =   0
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   4
      Left            =   1920
      Top             =   0
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   3
      Left            =   1440
      Top             =   0
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   2
      Left            =   960
      Top             =   0
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   1
      Left            =   480
      Top             =   0
      Width           =   495
   End
   Begin VB.Image imgTile 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Height          =   495
      Index           =   0
      Left            =   0
      Top             =   0
      Width           =   495
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim map(0 To 99, 0 To 99) As Integer
Dim brush As Integer

Public Sub refresh_map()

    'Dim i, j As Integer
    For i = 0 To 9
    For j = 0 To 9
        imgTile((10 * i) + j).Picture = pal(map(hsb.Value + j, vsb.Value + i)).Picture
    Next j
    Next i

End Sub

Private Sub cmdClear_Click()

    For i = 0 To 99
        imgTile(i).Picture = pal(brush).Picture
    Next i
    
    For i = 0 To 99
    For j = 0 To 99
            map(i, j) = brush
    Next j
    Next i

End Sub

Private Sub cmdLoad_Click()
    
    Dim location As String
    Dim incoming As String * 1

    location = InputBox("Enter file location to LOAD.", "LOAD")
    If location <> "" Then
        Open location For Binary As #1
        For j = 0 To 99
        For i = 0 To 99 Step 2
            Get #1, , incoming
            map(i, j) = Asc(incoming) / 16
            map(i + 1, j) = Asc(incoming) Mod 16
        Next i
        Next j
        
        For i = 0 To 46
            Get #1, , incoming
            txtObjs(i).Text = Val(Asc(incoming))
        Next i
        Close #1
        
        refresh_map
    End If

End Sub

Private Sub cmdNew_Click()

    For i = 0 To 99
    For j = 0 To 99
        map(i, j) = 0
    Next j
    Next i
    refresh_map

End Sub

Private Sub cmdRef_Click()

    refresh_map

End Sub

Private Sub cmdSave_Click()

    Dim location As String
    Dim objs As String

    location = InputBox("Enter file location to SAVE.", "SAVE")
    If location <> "" Then
        Open location For Output As #1
        For j = 0 To 99
        For i = 0 To 99 Step 2
            Print #1, Chr$(map(i, j) * 16 + map(i + 1, j));
        Next i
        Next j
        
        For i = 0 To 46
            Print #1, Chr$(Val(txtObjs(i)));
        Next i
        
        Close #1
    End If
    
End Sub


Private Sub Form_Load()

    refresh_map

End Sub

Private Sub hsb_Change()

    lblX.Caption = hsb.Value
    refresh_map

End Sub

Private Sub imgTile_Click(Index As Integer)

    map(hsb.Value + (Index Mod 10), vsb.Value + (Int(Index / 10))) = brush
    imgTile(Index).Picture = pal(brush).Picture

End Sub

Private Sub pal_Click(Index As Integer)

    shpSel.Top = pal(Index).Top
    brush = Index

End Sub

Private Sub vsb_Change()

    lblY.Caption = vsb.Value
    refresh_map

End Sub
