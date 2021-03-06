/////////////////////////////////////////////////////////////////////////////
// cmdSampleViewportDecoration.cpp : command file
//

#include "stdafx.h"
#include "SampleDisplayConduitsPlugIn.h"

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
//
// BEGIN SampleViewportDecoration command
//

class CSampleViewportDecorationEventWatcher : public CRhinoEventWatcher
{
public:
  CSampleViewportDecorationEventWatcher();
  ~CSampleViewportDecorationEventWatcher() {}

  void SetActiveView( CRhinoView* active_view );

  // CRhinoEventWatcher overrides
  void OnSetActiveView( CRhinoView* new_active_view );

private:
  CRhinoView* m_old_active_view;
};

CSampleViewportDecorationEventWatcher::CSampleViewportDecorationEventWatcher()
: m_old_active_view(0)
{
}

void CSampleViewportDecorationEventWatcher::SetActiveView( CRhinoView* active_view )
{
  m_old_active_view = active_view;
}

void CSampleViewportDecorationEventWatcher::OnSetActiveView( CRhinoView* new_active_view )
{
  // Regenerate the old active view (to remove decoration)
  if( m_old_active_view )
    m_old_active_view->Redraw( CRhinoView::regenerate_display_hint );

  // Regenerate the new active view (to draw decoration)
  if( new_active_view )
    new_active_view->Redraw( CRhinoView::regenerate_display_hint );

  m_old_active_view = new_active_view;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

class CSampleViewportDecorationConduit : public CRhinoDisplayConduit
{
public:
  CSampleViewportDecorationConduit();
  ~CSampleViewportDecorationConduit() {}
  bool ExecConduit(
        CRhinoDisplayPipeline& dp, // pipeline executing this conduit
        UINT nChannelID,           // current channel within the pipeline
        bool& bTerminationFlag     // channel termination flag
        );    
};

CSampleViewportDecorationConduit::CSampleViewportDecorationConduit()
: CRhinoDisplayConduit( CSupportChannels::SC_DRAWFOREGROUND )
{
}

bool CSampleViewportDecorationConduit::ExecConduit( CRhinoDisplayPipeline& dp, UINT nChannelID, bool& bTerminationFlag )
{
  // Get the active view
  CRhinoView* view = RhinoApp().ActiveView();
  if( 0 == view )
    return true;

  // Get the viewport that we are currently drawing in
  CRhinoViewport& vp = dp.GetRhinoVP();

  // Compare ids
  if( view->ActiveViewportID() != vp.ViewportId() )
    return true;

  if( nChannelID == CSupportChannels::SC_DRAWFOREGROUND )
  {
    // Use the screen port to determine text location
    int vp_left, vp_right, vp_top, vp_bottom;
    vp.VP().GetScreenPort( &vp_left, &vp_right, &vp_bottom, &vp_top );
    int vp_width = vp_right - vp_left;
    int vp_height = vp_bottom - vp_top;

    // Determine rect of text string
    ON_wString str( L"Hello Rhino!" );
    CRect rect;
    dp.MeasureString( rect, str, str.Length(), ON_2dPoint(0,0) );

    // Make sure text will fit on string
    int x_gap = 4;
    int y_gap = 4;
    if( rect.Width() + (2*x_gap) < vp_width ||  rect.Height() + (2*y_gap) < vp_height )
    {
      // Cook up text location (lower right corner of viewport)
      ON_2dPoint point( vp_right - rect.Width() - x_gap, vp_bottom - y_gap );

      // Draw text
      dp.DrawString( str, str.Length(), RGB(255,255,255), point );
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

class CCommandSampleViewportDecoration : public CRhinoCommand
{
public:
	CCommandSampleViewportDecoration();
	~CCommandSampleViewportDecoration() {}
	UUID CommandUUID()
	{
		// {9A827A94-B266-4D97-BD50-AE388C9E6FB0}
		static const GUID SampleViewportDecorationCommand_UUID =
		{ 0x9A827A94, 0xB266, 0x4D97, { 0xBD, 0x50, 0xAE, 0x38, 0x8C, 0x9E, 0x6F, 0xB0 } };
		return SampleViewportDecorationCommand_UUID;
	}
	const wchar_t* EnglishCommandName() { return L"SampleViewportDecoration"; }
	const wchar_t* LocalCommandName() { return L"SampleViewportDecoration"; }
	CRhinoCommand::result RunCommand( const CRhinoCommandContext& );

private:
  CSampleViewportDecorationConduit m_conduit;
  CSampleViewportDecorationEventWatcher m_watcher;
};

// The one and only CCommandSampleViewportDecoration object
static class CCommandSampleViewportDecoration theSampleViewportDecorationCommand;

CCommandSampleViewportDecoration::CCommandSampleViewportDecoration()
{
  // Register our event watcher
  m_watcher.Register();
}

CRhinoCommand::result CCommandSampleViewportDecoration::RunCommand( const CRhinoCommandContext& context )
{
  bool bEnable = m_conduit.IsEnabled();

  CRhinoGetOption go;
  go.SetCommandPrompt( L"Viewport decoration options" );
  go.AddCommandOptionToggle( RHCMDOPTNAME(L"Enable"), RHCMDOPTVALUE(L"No"), RHCMDOPTVALUE(L"Yes"), bEnable, &bEnable );
  go.AcceptNothing();
  for(;;)
  {
    CRhinoGet::result res = go.GetOption();

    if( res == CRhinoGet::option )
    {
      // Get the active view
      CRhinoView* view = RhinoApp().ActiveView();
      if( 0 == view )
      {
        RhinoApp().Print( L"Unable to obtain the active view.\n" );
        return CRhinoCommand::failure;
      }

      if( bEnable )
      {
        m_watcher.SetActiveView( view );
        m_watcher.Enable( TRUE );
        m_conduit.Enable();
      }
      else
      {
        m_watcher.SetActiveView( 0 );
        m_watcher.Enable( FALSE );
        m_conduit.Disable();
      }
      
      // Redraw the active view
      view->Redraw( CRhinoView::regenerate_display_hint );
      continue;
    }

    break;
  }

  return CRhinoCommand::success;
}

//
// END SampleViewportDecoration command
//
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
