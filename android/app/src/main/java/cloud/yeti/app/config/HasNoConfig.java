package cloud.yeti.app.config;

public class HasNoConfig extends Exception
{
    public HasNoConfig(String message)
    {
        super(message);
    }
}